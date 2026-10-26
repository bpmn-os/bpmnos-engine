#include "Object.h"
#include "Value.h"
#include "Schema.h"
#include "StringRegistry.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>

namespace BPMNOS {

Layout::Layout(std::shared_ptr<const Schema> schema)
  : variant(std::monostate{})
  , schema(std::move(schema))
  , size(1)
{
  assert( std::holds_alternative<ValueType>(*this->schema) );
}

Layout::Layout(std::shared_ptr<const Schema> schema, std::shared_ptr<const Layout> layout, size_t size)
  : variant(ArrayLayout{ layout, size })
  , schema(std::move(schema))
  , size(size * layout->size)
{
  assert( std::holds_alternative<ArrayType>(*this->schema) );
}

Layout::Layout(std::shared_ptr<const Schema> schema, const std::vector< std::shared_ptr<const Layout> >& fields)
  : variant(ObjectLayout{})
  , schema(std::move(schema))
  , size(0)
{
  assert( std::get<ObjectType>(*this->schema).fields.size() == fields.size() );
  // the fields are stored one after the other in the order of the schema
  auto& object = std::get<ObjectLayout>(*this);
  object.fields.reserve(fields.size());
  for ( auto& field : fields ) {
    object.fields.push_back( FieldLayout{ size, field } );
    size += field->size;
  }
}

std::string Layout::stringify() const {
  // the sizes of the arrays are written after the innermost elements, outermost first
  std::string sizes;
  const Layout* current = this;
  while ( auto array = std::get_if<ArrayLayout>(current) ) {
    sizes += "[" + std::to_string(array->size) + "]";
    current = array->layout.get();
  }
  if ( std::holds_alternative<std::monostate>(*current) ) {
    return current->schema->stringify() + sizes;
  }
  auto& fields = std::get<ObjectLayout>(*current).fields;
  auto& types = std::get<ObjectType>(*current->schema).fields;
  std::string result = "{ ";
  for ( size_t k = 0; k < fields.size(); k++ ) {
    result += ( k ? ", " : "" ) + types[k].name + ": " + fields[k].layout->stringify();
  }
  return result + " }" + sizes;
}

bool Layout::operator==(const Layout& other) const {
  if ( this == &other ) {
    return true;
  }
  if ( index() != other.index() || size != other.size || !( *schema == *other.schema ) ) {
    return false;
  }
  if ( auto array = std::get_if<ArrayLayout>(this) ) {
    auto& otherArray = std::get<ArrayLayout>(other);
    return array->size == otherArray.size && *array->layout == *otherArray.layout;
  }
  if ( auto object = std::get_if<ObjectLayout>(this) ) {
    return std::ranges::equal(object->fields, std::get<ObjectLayout>(other).fields, [](auto& field, auto& otherField) {
      return field.offset == otherField.offset && *field.layout == *otherField.layout;
    });
  }
  return true;
}

namespace {

/// Renders the values of a layout beginning at the given offset.
std::string render(const Layout& layout, const std::vector<Value>& values, size_t offset) {
  if ( auto array = std::get_if<ArrayLayout>(&layout) ) {
    if ( array->size == 0 ) {
      return "[ ]";
    }
    std::string result;
    for ( size_t i = 0; i < array->size; i++ ) {
      result += ", " + render(*array->layout, values, offset + i * array->layout->size);
    }
    result.front() = '[';
    return result + " ]";
  }
  if ( std::holds_alternative<std::monostate>(layout) ) {
    auto& value = values[offset];
    if ( !value.has_value() ) {
      return "undefined";
    }
    auto type = std::get<ValueType>(*layout.schema);
    auto text = BPMNOS::to_string(value.value(), type);
    return type == ValueType::STRING ? "\"" + text + "\"" : text;
  }
  auto& fields = std::get<ObjectLayout>(layout).fields;
  auto& types = std::get<ObjectType>(*layout.schema).fields;
  std::string result = "{ ";
  for ( size_t k = 0; k < fields.size(); k++ ) {
    result += ( k ? ", " : "" ) + types[k].name + " := " + render(*fields[k].layout, values, offset + fields[k].offset);
  }
  return result + " }";
}

/// Renders the values of a layout beginning at the given offset as JSON.
nlohmann::ordered_json json(const Layout& layout, const std::vector<Value>& values, size_t offset) {
  if ( auto array = std::get_if<ArrayLayout>(&layout) ) {
    auto result = nlohmann::ordered_json::array();
    for ( size_t i = 0; i < array->size; i++ ) {
      result.push_back( json(*array->layout, values, offset + i * array->layout->size) );
    }
    return result;
  }
  if ( std::holds_alternative<std::monostate>(layout) ) {
    auto& value = values[offset];
    if ( !value.has_value() ) {
      return nullptr;
    }
    switch ( std::get<ValueType>(*layout.schema) ) {
      case ValueType::BOOLEAN:
        return value.value() != number(0);
      case ValueType::INTEGER:
        return (long)value.value();
      case ValueType::DECIMAL:
        return (double)value.value();
      case ValueType::STRING:
        return BPMNOS::to_string(value.value(), ValueType::STRING);
    }
  }
  auto& fields = std::get<ObjectLayout>(layout).fields;
  auto& types = std::get<ObjectType>(*layout.schema).fields;
  auto result = nlohmann::ordered_json::object();
  for ( size_t k = 0; k < fields.size(); k++ ) {
    result[types[k].name] = json(*fields[k].layout, values, offset + fields[k].offset);
  }
  return result;
}

/**
 * Visits the values of an object of the layout `from` that an object of the layout `to`, of the same structure,
 * also has, calling `visit(source, target)` with the positions of each such value in the two objects.
 */
void correspond(const Layout& from, size_t fromOffset, const Layout& to, size_t toOffset, const std::function<void(size_t, size_t)>& visit) {
  if ( auto array = std::get_if<ArrayLayout>(&from) ) {
    auto& toArray = std::get<ArrayLayout>(to);
    size_t count = std::min(array->size, toArray.size);
    for ( size_t i = 0; i < count; i++ ) {
      correspond(*array->layout, fromOffset + i * array->layout->size, *toArray.layout, toOffset + i * toArray.layout->size, visit);
    }
    return;
  }
  if ( std::holds_alternative<std::monostate>(from) ) {
    visit(fromOffset, toOffset);
    return;
  }
  auto& fields = std::get<ObjectLayout>(from).fields;
  auto& toFields = std::get<ObjectLayout>(to).fields;
  for ( size_t k = 0; k < fields.size(); k++ ) {
    correspond(*fields[k].layout, fromOffset + fields[k].offset, *toFields[k].layout, toOffset + toFields[k].offset, visit);
  }
}

/// Returns the layouts of the fields of an object layout.
std::vector< std::shared_ptr<const Layout> > fieldLayoutsOf(const Layout& layout) {
  std::vector< std::shared_ptr<const Layout> > result;
  for ( auto& field : std::get<ObjectLayout>(layout).fields ) {
    result.push_back(field.layout);
  }
  return result;
}

/// Returns the layout in which the array the steps reach from the given one on has the given size, rebuilding the
/// layouts and, where a fixed array changes its size, the schemas along the steps.
std::shared_ptr<const Layout> rebuild(const std::shared_ptr<const Layout>& layout, const std::vector< std::optional<std::string> >& steps, size_t step, size_t size) {
  if ( step == steps.size() ) {
    auto& array = std::get<ArrayLayout>(*layout);
    auto& type = std::get<ArrayType>(*layout->schema);
    // an array fixed in the schema stays fixed at its new size
    auto schema = type.size.has_value() ? std::make_shared<const Schema>( ArrayType{ type.type, size } ) : layout->schema;
    return std::make_shared<const Layout>(schema, array.layout, size);
  }
  if ( !steps[step].has_value() ) {
    auto& array = std::get<ArrayLayout>(*layout);
    auto element = rebuild(array.layout, steps, step + 1, size);
    auto& type = std::get<ArrayType>(*layout->schema);
    auto schema = ( element->schema == array.layout->schema ) ? layout->schema : std::make_shared<const Schema>( ArrayType{ element->schema, type.size } );
    return std::make_shared<const Layout>(schema, element, array.size);
  }
  auto& name = steps[step].value();
  auto type = std::get<ObjectType>(*layout->schema);
  auto field = (size_t)std::distance(type.fields.begin(), std::ranges::find_if(type.fields, [&name](auto& candidate) { return candidate.name == name; }));
  if ( field == type.fields.size() ) {
    throw std::logic_error("Object: unknown field '" + name + "'");
  }
  auto fields = fieldLayoutsOf(*layout);
  auto changed = rebuild(fields[field], steps, step + 1, size);
  auto schema = layout->schema;
  if ( changed->schema != fields[field]->schema ) {
    type.fields[field].type = changed->schema;
    schema = std::make_shared<const Schema>( std::move(type) );
  }
  fields[field] = std::move(changed);
  return std::make_shared<const Layout>(schema, fields);
}

/// Returns the layout in which every array has the largest size it has in the two layouts, and is fixed only where
/// both fix it at the same size.
std::shared_ptr<const Layout> widen(const std::shared_ptr<const Layout>& layout, const std::shared_ptr<const Layout>& other) {
  if ( layout == other || *layout == *other || std::holds_alternative<std::monostate>(*layout) ) {
    return layout;
  }
  if ( auto array = std::get_if<ArrayLayout>(layout.get()) ) {
    auto& otherArray = std::get<ArrayLayout>(*other);
    auto element = widen(array->layout, otherArray.layout);
    auto& type = std::get<ArrayType>(*layout->schema);
    auto& otherType = std::get<ArrayType>(*other->schema);
    auto fixed = ( type.size.has_value() && type.size == otherType.size ) ? type.size : std::nullopt;
    auto schema = ( element->schema == array->layout->schema && fixed == type.size ) ? layout->schema : std::make_shared<const Schema>( ArrayType{ element->schema, fixed } );
    return std::make_shared<const Layout>(schema, element, std::max(array->size, otherArray.size));
  }
  auto fields = fieldLayoutsOf(*layout);
  auto otherFields = fieldLayoutsOf(*other);
  auto type = std::get<ObjectType>(*layout->schema);
  bool changed = false;
  for ( size_t k = 0; k < fields.size(); k++ ) {
    auto widened = widen(fields[k], otherFields[k]);
    if ( widened->schema != fields[k]->schema ) {
      type.fields[k].type = widened->schema;
      changed = true;
    }
    fields[k] = std::move(widened);
  }
  return std::make_shared<const Layout>( changed ? std::make_shared<const Schema>( std::move(type) ) : layout->schema, fields );
}

} // namespace

Object& modifiable(std::shared_ptr<const Object>& slot) {
  if ( slot.use_count() != 1 ) {
    slot = std::make_shared<Object>(*slot);
  }
  // an object held by the slot alone was created modifiable, a constant object being held by the registry too
  return const_cast<Object&>(*slot);
}

Location locate(const std::shared_ptr<const Layout>& layout, const std::vector< std::optional<std::string> >& steps, std::span<const size_t> indices) {
  Location location{ &layout, 0 };
  size_t next = 0;
  for ( auto& step : steps ) {
    auto& current = **location.layout;
    if ( step.has_value() ) {
      auto& types = std::get<ObjectType>(*current.schema).fields;
      auto field = (size_t)std::distance(types.begin(), std::ranges::find_if(types, [&step](auto& candidate) { return candidate.name == step.value(); }));
      if ( field == types.size() ) {
        throw std::logic_error("Object: unknown field '" + step.value() + "'");
      }
      auto& fieldLayout = std::get<ObjectLayout>(current).fields[field];
      location = Location{ &fieldLayout.layout, location.offset + fieldLayout.offset };
      continue;
    }
    auto index = indices[next++];
    auto& array = std::get<ArrayLayout>(current);
    if ( index >= array.size ) {
      throw std::out_of_range("Object: array of size " + std::to_string(array.size) + " has no element " + std::to_string(index + 1));
    }
    location = Location{ &array.layout, location.offset + index * array.layout->size };
  }
  assert( next == indices.size() );
  return location;
}

std::shared_ptr<Object> resize(const Object& object, const std::vector< std::optional<std::string> >& steps, size_t size) {
  auto result = std::make_shared<Object>();
  result->layout = rebuild(object.layout, steps, 0, size);
  result->values.resize(result->layout->size);
  correspond(*object.layout, 0, *result->layout, 0, [&](size_t source, size_t target) {
    result->values[target] = object.values[source];
  });
  return result;
}

std::shared_ptr<const Object> merge(const std::vector< std::shared_ptr<const Object> >& objects) {
  assert( !objects.empty() );
  if ( std::ranges::all_of(objects, [&objects](auto& object) { return object == objects.front(); }) ) {
    return objects.front();
  }
  auto layout = objects.front()->layout;
  for ( auto& object : objects ) {
    layout = widen(layout, object->layout);
  }
  auto result = std::make_shared<Object>();
  result->values.resize(layout->size);
  // a value is merged as a status value is: the first defined value is kept, and a conflict with it makes the
  // value undefined for good
  std::vector<bool> conflict(layout->size, false);
  for ( auto& object : objects ) {
    correspond(*object->layout, 0, *layout, 0, [&](size_t source, size_t target) {
      auto& value = object->values[source];
      auto& merged = result->values[target];
      if ( conflict[target] ) {
        return;
      }
      if ( !merged.has_value() ) {
        merged = value;
      }
      else if ( value.has_value() && value.value() != merged.value() ) {
        merged = std::nullopt;
        conflict[target] = true;
      }
    });
  }
  result->layout = std::move(layout);
  return result;
}

std::string to_string(const Object& object) {
  return render(*object.layout, object.values, 0);
}

nlohmann::ordered_json to_json(const Object& object) {
  return json(*object.layout, object.values, 0);
}

namespace {

/// Returns the number of arrays a schema has before its innermost elements.
size_t countArrays(const Schema& schema) {
  size_t count = 0;
  for ( auto current = &schema; auto array = std::get_if<ArrayType>(current); current = array->type.get() ) {
    count++;
  }
  return count;
}

/// Returns the layout of the object converted to the schema from a source whose layout the given pointer holds, or, if
/// no source is given, of an object of the schema with every open array having size 0. Every part of the layout equal
/// to the corresponding part of the layout of the source is that part, so that it is shared.
std::shared_ptr<const Layout> determineLayout(const std::shared_ptr<const Schema>& schema, const std::shared_ptr<const Layout>* source, const std::string& path) {
  auto share = [source](std::shared_ptr<const Layout> layout) {
    return ( source && *layout == **source ) ? *source : layout;
  };
  const Schema* sourceSchema = source ? (*source)->schema.get() : nullptr;
  if ( sourceSchema && countArrays(*sourceSchema) != countArrays(*schema) ) {
    throw std::runtime_error("Object: '" + path + "' has " + std::to_string(countArrays(*sourceSchema)) + " dimensions instead of " + std::to_string(countArrays(*schema)));
  }
  if ( auto array = std::get_if<ArrayType>(schema.get()) ) {
    // a fixed array has its size, an open array the size of the source
    auto sourceArray = source ? &std::get<ArrayLayout>(**source) : nullptr;
    auto element = determineLayout(array->type, sourceArray ? &sourceArray->layout : nullptr, path);
    return share( std::make_shared<const Layout>(schema, element, array->size.value_or( sourceArray ? sourceArray->size : 0 )) );
  }
  if ( auto valueType = std::get_if<ValueType>(schema.get()) ) {
    if ( sourceSchema ) {
      auto sourceType = std::get_if<ValueType>(sourceSchema);
      if ( !sourceType ) {
        throw std::runtime_error("Object: '" + path + "' has fields where a value is expected");
      }
      // arrays and objects are not cast
      if ( *valueType != *sourceType ) {
        throw std::runtime_error("Object: '" + path + "' cannot be converted to the type of the attribute");
      }
    }
    return share( std::make_shared<const Layout>(schema) );
  }
  auto& fields = std::get<ObjectType>(*schema).fields;
  const std::vector<Field>* sourceFields = nullptr;
  if ( sourceSchema ) {
    if ( std::holds_alternative<ValueType>(*sourceSchema) ) {
      throw std::runtime_error("Object: '" + path + "' is a value where fields are expected");
    }
    sourceFields = &std::get<ObjectType>(*sourceSchema).fields;
    for ( auto& sourceField : *sourceFields ) {
      if ( std::ranges::none_of(fields, [&sourceField](auto& field) { return field.name == sourceField.name; }) ) {
        throw std::runtime_error("Object: '" + path + "' has the unknown field '" + sourceField.name + "'");
      }
    }
  }
  std::vector< std::shared_ptr<const Layout> > layouts;
  layouts.reserve(fields.size());
  for ( auto& field : fields ) {
    const std::shared_ptr<const Layout>* sourceField = nullptr;
    if ( sourceFields ) {
      if ( auto it = std::ranges::find_if(*sourceFields, [&field](auto& candidate) { return candidate.name == field.name; }); it != sourceFields->end() ) {
        sourceField = &std::get<ObjectLayout>(**source).fields[(size_t)std::distance(sourceFields->begin(), it)].layout;
      }
    }
    layouts.push_back( determineLayout(field.type, sourceField, path + "." + field.name) );
  }
  return share( std::make_shared<const Layout>(schema, layouts) );
}

/// Copies the values of a source of the layout `from` into an object of the layout `to`, and rejects a source whose
/// elements beyond the size of a fixed array hold a defined value.
void copy(const Layout& from, const std::vector<Value>& source, size_t sourceOffset, const Layout& to, std::vector<Value>& target, size_t targetOffset, const std::string& path) {
  if ( auto array = std::get_if<ArrayLayout>(&from) ) {
    auto& toArray = std::get<ArrayLayout>(to);
    size_t span = array->layout->size;
    for ( size_t i = 0; i < array->size; i++ ) {
      if ( i < toArray.size ) {
        copy(*array->layout, source, sourceOffset + i * span, *toArray.layout, target, targetOffset + i * toArray.layout->size, path);
      }
      else if ( std::any_of(source.begin() + (long)(sourceOffset + i * span), source.begin() + (long)(sourceOffset + (i + 1) * span), [](auto& value) { return value.has_value(); }) ) {
        // only undefined values are cut off
        throw std::runtime_error("Object: '" + path + "' has " + std::to_string(array->size) + " elements where " + std::to_string(toArray.size) + " are declared");
      }
    }
    return;
  }
  if ( std::holds_alternative<std::monostate>(from) ) {
    target[targetOffset] = source[sourceOffset];
    return;
  }
  auto& fields = std::get<ObjectLayout>(from).fields;
  auto& types = std::get<ObjectType>(*from.schema).fields;
  auto& toFields = std::get<ObjectLayout>(to).fields;
  auto& toTypes = std::get<ObjectType>(*to.schema).fields;
  for ( size_t k = 0; k < fields.size(); k++ ) {
    auto& name = types[k].name;
    auto it = std::ranges::find_if(toTypes, [&name](auto& candidate) { return candidate.name == name; });
    auto& toField = toFields[(size_t)std::distance(toTypes.begin(), it)];
    copy(*fields[k].layout, source, sourceOffset + fields[k].offset, *toField.layout, target, targetOffset + toField.offset, path + "." + name);
  }
}

using Json = nlohmann::json;

/// Returns the layout of an object of the schema holding the JSON values standing where the schema applies, every
/// array at an open array having the same size.
std::shared_ptr<const Layout> determineLayout(const std::shared_ptr<const Schema>& schema, const std::vector<const Json*>& values, const std::string& path) {
  if ( auto array = std::get_if<ArrayType>(schema.get()) ) {
    std::optional<size_t> size;
    std::vector<const Json*> elements;
    for ( auto value : values ) {
      if ( value->is_null() ) {
        continue;
      }
      if ( !value->is_array() ) {
        throw std::runtime_error("Object: '" + path + "' holds '" + value->dump() + "' where an array is expected");
      }
      if ( array->size.has_value() ) {
        if ( value->size() > array->size.value() ) {
          throw std::runtime_error("Object: '" + path + "' has " + std::to_string(value->size()) + " elements where " + std::to_string(array->size.value()) + " are declared");
        }
      }
      else if ( size.has_value() && value->size() != size.value() ) {
        throw std::runtime_error("Object: '" + path + "' holds arrays of different sizes");
      }
      size = value->size();
      for ( auto& element : *value ) {
        elements.push_back(&element);
      }
    }
    auto element = determineLayout(array->type, elements, path);
    return std::make_shared<const Layout>(schema, element, array->size.value_or( size.value_or(0) ));
  }
  if ( std::holds_alternative<ValueType>(*schema) ) {
    return std::make_shared<const Layout>(schema);
  }
  auto& fields = std::get<ObjectType>(*schema).fields;
  for ( auto value : values ) {
    if ( value->is_null() ) {
      continue;
    }
    if ( !value->is_object() ) {
      throw std::runtime_error("Object: '" + path + "' holds '" + value->dump() + "' where fields are expected");
    }
    for ( auto& [key, _] : value->items() ) {
      if ( std::ranges::none_of(fields, [&key](auto& field) { return field.name == key; }) ) {
        throw std::runtime_error("Object: '" + path + "' has the unknown field '" + key + "'");
      }
    }
  }
  std::vector< std::shared_ptr<const Layout> > layouts;
  layouts.reserve(fields.size());
  for ( auto& field : fields ) {
    std::vector<const Json*> nested;
    for ( auto value : values ) {
      if ( !value->is_null() ) {
        if ( auto it = value->find(field.name); it != value->end() ) {
          nested.push_back(&*it);
        }
      }
    }
    layouts.push_back( determineLayout(field.type, nested, path + "." + field.name) );
  }
  return std::make_shared<const Layout>(schema, layouts);
}

/// Returns the single value a JSON value states for the given value type.
Value convert(const Json& value, ValueType type, const std::string& path) {
  if ( value.is_null() ) {
    return std::nullopt;
  }
  switch ( type ) {
    case ValueType::BOOLEAN:
      if ( value.is_boolean() ) {
        return number( value.get<bool>() ? 1 : 0 );
      }
      break;
    case ValueType::INTEGER:
      if ( value.is_number() ) {
        return number( (long)value.get<double>() );
      }
      break;
    case ValueType::DECIMAL:
      if ( value.is_number() ) {
        return number( value.get<double>() );
      }
      break;
    case ValueType::STRING:
      if ( value.is_string() ) {
        return number( stringRegistry( value.get<std::string>() ) );
      }
      break;
  }
  throw std::runtime_error("Object: '" + path + "' holds '" + value.dump() + "', which is no value of its type");
}

/// Writes the values a JSON value states into an object of the given layout, beginning at the given offset.
void fill(const Layout& layout, const Json& value, std::vector<Value>& values, size_t offset, const std::string& path) {
  if ( value.is_null() ) {
    return;
  }
  if ( auto array = std::get_if<ArrayLayout>(&layout) ) {
    size_t i = 0;
    for ( auto& element : value ) {
      fill(*array->layout, element, values, offset + i * array->layout->size, path);
      ++i;
    }
    return;
  }
  if ( std::holds_alternative<std::monostate>(layout) ) {
    values[offset] = convert(value, std::get<ValueType>(*layout.schema), path);
    return;
  }
  auto& fields = std::get<ObjectLayout>(layout).fields;
  auto& types = std::get<ObjectType>(*layout.schema).fields;
  for ( size_t k = 0; k < fields.size(); k++ ) {
    if ( auto it = value.find(types[k].name); it != value.end() ) {
      fill(*fields[k].layout, *it, values, offset + fields[k].offset, path + "." + types[k].name);
    }
  }
}

} // namespace

Object::Object(const Schema& schema)
  : layout( determineLayout(std::make_shared<const Schema>(schema), nullptr, "") )
  , values( layout->size )
{
}

Object::Object(const Schema& schema, const nlohmann::json& json)
  : layout( determineLayout(std::make_shared<const Schema>(schema), std::vector<const Json*>{ &json }, "") )
  , values( layout->size )
{
  fill(*layout, json, values, 0, "");
}

Object::Object(const Schema& schema, const Object& source) {
  auto path = to_string(source);
  layout = determineLayout(std::make_shared<const Schema>(schema), &source.layout, path);
  values.resize( layout->size );
  copy(*source.layout, source.values, 0, *layout, values, 0, path);
}

} // namespace BPMNOS
