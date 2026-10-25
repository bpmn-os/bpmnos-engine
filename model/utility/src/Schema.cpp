#include "Schema.h"
#include "model/utility/src/StringRegistry.h"
#include <nlohmann/json.hpp>
#include <cctype>
#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>
#include <variant>

using namespace BPMNOS;

namespace {

/**
 * @brief A schema in the form today's layouts take: a value type or fields, followed by the sizes of the arrays
 * around it, outermost first.
 *
 * The functions building and reading layouts work on this form, into which a schema is flattened.
 */
struct FlatSchema {
  std::optional<ValueType> scalar;
  std::vector< std::pair<std::string, FlatSchema> > fields;
  std::vector< std::optional<size_t> > dimensions;
};

/// Returns the flat form of a schema.
FlatSchema flatten(const Schema& schema) {
  FlatSchema result;
  const Schema* current = &schema;
  while ( auto array = std::get_if<ArrayType>(current) ) {
    result.dimensions.push_back(array->size);
    current = array->type.get();
  }
  if ( auto valueType = std::get_if<ValueType>(current) ) {
    result.scalar = *valueType;
  }
  else {
    for ( auto& field : std::get<ObjectType>(*current).fields ) {
      result.fields.emplace_back(field.name, flatten(*field.type));
    }
  }
  return result;
}

/**
 * @brief Recursive descent parser of the notation `schema := ( valuetype | object ) dimension*`.
 */
class SchemaParser {
public:
  explicit SchemaParser(const std::string& text) : text(text), position(0) {}

  Schema parseType() {
    skipWhitespace();
    Schema result = ( peek() == '{' ) ? Schema(parseObject()) : Schema(parseValueType());
    skipWhitespace();
    std::vector< std::optional<size_t> > sizes;
    while ( peek() == '[' ) {
      ++position;
      skipWhitespace();
      if ( peek() == ']' ) {
        sizes.push_back(std::nullopt);
      }
      else {
        sizes.push_back(parseSize());
      }
      skipWhitespace();
      expect(']');
      skipWhitespace();
    }
    // the dimensions are written outermost first, so that the innermost array is built first
    for ( auto it = sizes.rbegin(); it != sizes.rend(); ++it ) {
      result = Schema( ArrayType{ std::make_shared<const Schema>(std::move(result)), *it } );
    }
    return result;
  }

  void expectEnd() {
    skipWhitespace();
    if ( position != text.size() ) {
      fail("unexpected '" + text.substr(position) + "'");
    }
  }

private:
  const std::string& text;
  size_t position;

  char peek() const {
    return position < text.size() ? text[position] : '\0';
  }

  void skipWhitespace() {
    while ( position < text.size() && std::isspace((unsigned char)text[position]) ) {
      ++position;
    }
  }

  [[noreturn]] void fail(const std::string& reason) const {
    throw std::runtime_error("Schema: illegal type '" + text + "': " + reason);
  }

  void expect(char character) {
    if ( peek() != character ) {
      fail(std::string("expected '") + character + "' at position " + std::to_string(position + 1));
    }
    ++position;
  }

  std::string parseName() {
    size_t start = position;
    if ( !( std::isalpha((unsigned char)peek()) || peek() == '_' ) ) {
      fail("expected a name at position " + std::to_string(position + 1));
    }
    while ( position < text.size() && ( std::isalnum((unsigned char)text[position]) || text[position] == '_' ) ) {
      ++position;
    }
    return text.substr(start, position - start);
  }

  ValueType parseValueType() {
    std::string name = parseName();
    if ( name == "boolean" ) return ValueType::BOOLEAN;
    if ( name == "integer" ) return ValueType::INTEGER;
    if ( name == "decimal" ) return ValueType::DECIMAL;
    if ( name == "string" ) return ValueType::STRING;
    fail("unknown type '" + name + "'");
  }

  size_t parseSize() {
    size_t start = position;
    while ( position < text.size() && std::isdigit((unsigned char)text[position]) ) {
      ++position;
    }
    if ( position == start ) {
      fail("expected a size at position " + std::to_string(position + 1));
    }
    size_t size = std::stoul(text.substr(start, position - start));
    if ( size == 0 ) {
      fail("a size must be positive");
    }
    return size;
  }

  ObjectType parseObject() {
    ObjectType object;
    expect('{');
    skipWhitespace();
    if ( peek() == '}' ) {
      fail("an object requires at least one field");
    }
    while ( true ) {
      skipWhitespace();
      std::string name = parseName();
      for ( auto& field : object.fields ) {
        if ( field.name == name ) {
          fail("duplicate field '" + name + "'");
        }
      }
      skipWhitespace();
      expect(':');
      object.fields.push_back( Field{ name, std::make_shared<const Schema>(parseType()) } );
      skipWhitespace();
      if ( peek() == ',' ) {
        ++position;
        continue;
      }
      expect('}');
      return object;
    }
  }
};

} // namespace

Schema Schema::parse(const std::string& text) {
  SchemaParser parser(text);
  Schema schema = parser.parseType();
  parser.expectEnd();
  return schema;
}

std::string Schema::stringify() const {
  // the sizes of the arrays are written after the type of the innermost elements, outermost first
  std::string sizes;
  const Schema* current = this;
  while ( auto array = std::get_if<ArrayType>(current) ) {
    sizes += array->size.has_value() ? "[" + std::to_string(array->size.value()) + "]" : "[]";
    current = array->type.get();
  }
  std::string result;
  if ( auto valueType = std::get_if<ValueType>(current) ) {
    switch ( *valueType ) {
      case ValueType::BOOLEAN: result = "boolean"; break;
      case ValueType::INTEGER: result = "integer"; break;
      case ValueType::DECIMAL: result = "decimal"; break;
      case ValueType::STRING: result = "string"; break;
    }
  }
  else {
    auto& fields = std::get<ObjectType>(*current).fields;
    result = "{ ";
    for ( size_t i = 0; i < fields.size(); i++ ) {
      result += ( i ? ", " : "" ) + fields[i].name + ": " + fields[i].type->stringify();
    }
    result += " }";
  }
  return result + sizes;
}

bool Schema::operator==(const Schema& other) const {
  if ( index() != other.index() ) {
    return false;
  }
  if ( auto valueType = std::get_if<ValueType>(this) ) {
    return *valueType == std::get<ValueType>(other);
  }
  if ( auto array = std::get_if<ArrayType>(this) ) {
    auto& otherArray = std::get<ArrayType>(other);
    return array->size == otherArray.size && *array->type == *otherArray.type;
  }
  auto& fields = std::get<ObjectType>(*this).fields;
  auto& otherFields = std::get<ObjectType>(other).fields;
  return std::ranges::equal(fields, otherFields, [](auto& field, auto& otherField) {
    return field.name == otherField.name && *field.type == *otherField.type;
  });
}

bool Schema::supports(const Schema& source) const {
  if ( index() != source.index() ) {
    return false;
  }
  if ( auto valueType = std::get_if<ValueType>(this) ) {
    auto sourceType = std::get<ValueType>(source);
    return *valueType == sourceType ||
      ( *valueType == ValueType::INTEGER && sourceType == ValueType::BOOLEAN ) ||
      ( *valueType == ValueType::DECIMAL && ( sourceType == ValueType::BOOLEAN || sourceType == ValueType::INTEGER ) );
  }
  if ( auto array = std::get_if<ArrayType>(this) ) {
    auto& sourceArray = std::get<ArrayType>(source);
    return ( !array->size.has_value() || array->size == sourceArray.size ) && array->type->supports(*sourceArray.type);
  }
  auto& fields = std::get<ObjectType>(*this).fields;
  auto& sourceFields = std::get<ObjectType>(source).fields;
  return std::ranges::equal(fields, sourceFields, [](auto& field, auto& sourceField) {
    return field.name == sourceField.name && field.type->supports(*sourceField.type);
  });
}

namespace {

/// Returns the target schema for the effective schema and the current layout, the arrays of the schema corresponding
/// to the dimensions of the layout from the given one on.
Schema targetSchema(const Schema& effectiveSchema, const BPMNOS::Object::Layout& currentLayout, size_t dimension) {
  if ( auto array = std::get_if<ArrayType>(&effectiveSchema) ) {
    assert( dimension < currentLayout.dimensions.size() );
    auto size = array->size.has_value() ? std::optional<size_t>(currentLayout.dimensions[dimension]) : std::nullopt;
    return Schema( ArrayType{ std::make_shared<const Schema>( targetSchema(*array->type, currentLayout, dimension + 1) ), size } );
  }
  if ( std::holds_alternative<ValueType>(effectiveSchema) ) {
    return effectiveSchema;
  }
  ObjectType object;
  for ( auto& field : std::get<ObjectType>(effectiveSchema).fields ) {
    auto current = std::ranges::find_if(currentLayout.fields, [&field](auto& candidate) { return candidate.name == field.name; });
    assert( current != currentLayout.fields.end() );
    object.fields.push_back( Field{ field.name, std::make_shared<const Schema>( targetSchema(*field.type, current->layout, 0) ) } );
  }
  return Schema( std::move(object) );
}

} // namespace

Schema::Schema(const Schema& effectiveSchema, const BPMNOS::Object::Layout& currentLayout)
  : Schema( targetSchema(effectiveSchema, currentLayout, 0) )
{
}

Schema Schema::declareSizes(const std::string& declaration) const {
  auto error = [&declaration](const std::string& reason) {
    return std::runtime_error("Schema: illegal size declaration '" + declaration + "': " + reason);
  };
  // the steps of the declaration: the size an index states, std::nullopt for `[]`, or the name of a field
  std::vector< std::variant< std::optional<size_t>, std::string > > steps;
  size_t position = 0;
  auto skipWhitespace = [&]() {
    while ( position < declaration.size() && std::isspace((unsigned char)declaration[position]) ) {
      ++position;
    }
  };
  while ( true ) {
    skipWhitespace();
    if ( position == declaration.size() ) {
      break;
    }
    if ( declaration[position] == '[' ) {
      ++position;
      skipWhitespace();
      size_t start = position;
      while ( position < declaration.size() && std::isdigit((unsigned char)declaration[position]) ) {
        ++position;
      }
      std::optional<size_t> size;
      if ( position > start ) {
        size = std::stoul(declaration.substr(start, position - start));
        if ( size.value() == 0 ) {
          throw error("a size must be positive");
        }
      }
      skipWhitespace();
      if ( position == declaration.size() || declaration[position] != ']' ) {
        throw error("expected ']'");
      }
      ++position;
      steps.emplace_back(size);
    }
    else if ( declaration[position] == '.' ) {
      ++position;
      size_t start = position;
      while ( position < declaration.size() && ( std::isalnum((unsigned char)declaration[position]) || declaration[position] == '_' ) ) {
        ++position;
      }
      steps.emplace_back( declaration.substr(start, position - start) );
    }
    else {
      throw error("unexpected '" + declaration.substr(position) + "'");
    }
  }

  // the parts along the path are built anew, every other part is shared
  std::function<Schema(const Schema&, size_t)> apply = [&](const Schema& schema, size_t step) -> Schema {
    if ( step == steps.size() ) {
      return schema;
    }
    if ( auto size = std::get_if< std::optional<size_t> >(&steps[step]) ) {
      auto array = std::get_if<ArrayType>(&schema);
      if ( !array ) {
        throw error("more dimensions than the type has");
      }
      auto declared = array->size;
      if ( size->has_value() ) {
        if ( declared.has_value() && declared.value() != size->value() ) {
          throw error("size " + std::to_string(size->value()) + " contradicts size " + std::to_string(declared.value()));
        }
        declared = size->value();
      }
      return Schema( ArrayType{ std::make_shared<const Schema>(apply(*array->type, step + 1)), declared } );
    }
    auto& name = std::get<std::string>(steps[step]);
    if ( auto array = std::get_if<ArrayType>(&schema) ) {
      // a field is reached through the arrays around its object, whose sizes stay as they are
      return Schema( ArrayType{ std::make_shared<const Schema>(apply(*array->type, step)), array->size } );
    }
    auto object = std::get_if<ObjectType>(&schema);
    if ( !object ) {
      throw error("unknown field '" + name + "'");
    }
    ObjectType result = *object;
    auto field = std::ranges::find_if(result.fields, [&name](auto& candidate) { return candidate.name == name; });
    if ( field == result.fields.end() ) {
      throw error("unknown field '" + name + "'");
    }
    field->type = std::make_shared<const Schema>(apply(*field->type, step + 1));
    return Schema(std::move(result));
  };
  return apply(*this, 0);
}

namespace {

/// Returns the layout of a schema whose dimensions all have sizes, a dimension being fixed if it has a size in
/// the schema declared, of which the schema is a resolution.
BPMNOS::Object::Layout layoutOf(const FlatSchema& schema, const FlatSchema& declared) {
  BPMNOS::Object::Layout layout;
  layout.scalar = schema.scalar;
  layout.fields.reserve(schema.fields.size());
  for ( size_t k = 0; k < schema.fields.size(); k++ ) {
    layout.fields.push_back( BPMNOS::Object::Layout::Field{ schema.fields[k].first, 0, layoutOf(schema.fields[k].second, declared.fields[k].second) } );
  }
  layout.dimensions.reserve(schema.dimensions.size());
  layout.fixed.reserve(schema.dimensions.size());
  for ( size_t d = 0; d < schema.dimensions.size(); d++ ) {
    assert( schema.dimensions[d].has_value() );
    layout.dimensions.push_back( schema.dimensions[d].value() );
    layout.fixed.push_back( declared.dimensions[d].has_value() );
  }
  layout.arrange();
  return layout;
}

/// Gives the dimensions still open, of a field the value lacks or below an empty array, the length zero, which
/// leaves them undetermined until the first assignment of a value to one of their elements.
void close(FlatSchema& schema) {
  for ( auto& dimension : schema.dimensions ) {
    if ( !dimension.has_value() ) {
      dimension = 0;
    }
  }
  for ( auto& [name, field] : schema.fields ) {
    close(field);
  }
}

} // namespace

std::shared_ptr<const BPMNOS::Object> Schema::undefinedObject() const {
  // an object is created modifiable, so that a write may change it in place once it is no longer shared
  // an open dimension has no elements
  FlatSchema declared = flatten(*this);
  FlatSchema closed = declared;
  close(closed);
  auto object = std::make_shared<BPMNOS::Object>();
  object->layout = std::make_shared<const BPMNOS::Object::Layout>( layoutOf(closed, declared) );
  object->values.resize( object->layout->size() );
  return object;
}


namespace {

using Json = nlohmann::json;

/// Fixes the open dimensions of a schema by the JSON values standing where it applies, every array at an open
/// dimension having the same length.
void resolveJson(FlatSchema& schema, const std::vector<const Json*>& values, const std::string& path) {
  std::vector<const Json*> level = values;
  for ( auto& dimension : schema.dimensions ) {
    std::optional<size_t> length;
    std::vector<const Json*> next;
    for ( auto value : level ) {
      if ( value->is_null() ) {
        continue;
      }
      if ( !value->is_array() ) {
        throw std::runtime_error("Schema: '" + path + "' holds '" + value->dump() + "' where an array is expected");
      }
      if ( dimension.has_value() ) {
        if ( value->size() > dimension.value() ) {
          throw std::runtime_error("Schema: '" + path + "' has " + std::to_string(value->size()) + " elements where " + std::to_string(dimension.value()) + " are declared");
        }
      }
      else if ( length.has_value() && value->size() != length.value() ) {
        throw std::runtime_error("Schema: '" + path + "' holds arrays of different lengths");
      }
      length = value->size();
      for ( auto& element : *value ) {
        next.push_back(&element);
      }
    }
    if ( !dimension.has_value() ) {
      dimension = length.value_or(0);
    }
    level = std::move(next);
  }
  if ( schema.scalar.has_value() ) {
    return;
  }
  for ( auto value : level ) {
    if ( value->is_null() ) {
      continue;
    }
    if ( !value->is_object() ) {
      throw std::runtime_error("Schema: '" + path + "' holds '" + value->dump() + "' where fields are expected");
    }
    for ( auto& [key, _] : value->items() ) {
      if ( std::ranges::none_of(schema.fields, [&key](auto& field) { return field.first == key; }) ) {
        throw std::runtime_error("Schema: '" + path + "' has the unknown field '" + key + "'");
      }
    }
  }
  for ( auto& [name, field] : schema.fields ) {
    std::vector<const Json*> nested;
    for ( auto value : level ) {
      if ( !value->is_null() ) {
        if ( auto it = value->find(name); it != value->end() ) {
          nested.push_back(&*it);
        }
      }
    }
    resolveJson(field, nested, path + "." + name);
  }
}

/// Returns the value a JSON value states for a scalar of the given type.
BPMNOS::Value jsonValue(const Json& value, BPMNOS::ValueType type, const std::string& path) {
  if ( value.is_null() ) {
    return std::nullopt;
  }
  switch ( type ) {
    case BPMNOS::ValueType::BOOLEAN:
      if ( value.is_boolean() ) {
        return BPMNOS::number( value.get<bool>() ? 1 : 0 );
      }
      break;
    case BPMNOS::ValueType::INTEGER:
      if ( value.is_number() ) {
        return BPMNOS::number( (long)value.get<double>() );
      }
      break;
    case BPMNOS::ValueType::DECIMAL:
      if ( value.is_number() ) {
        return BPMNOS::number( value.get<double>() );
      }
      break;
    case BPMNOS::ValueType::STRING:
      if ( value.is_string() ) {
        return BPMNOS::number( stringRegistry( value.get<std::string>() ) );
      }
      break;
  }
  throw std::runtime_error("Schema: '" + path + "' holds '" + value.dump() + "', which is no value of its type");
}

/// Writes the values a JSON value states into an object of the given layout.
void fillJson(const BPMNOS::Object::Layout& layout, const Json& value, std::vector<BPMNOS::Value>& values, size_t offset, size_t dimension, const std::string& path) {
  if ( value.is_null() ) {
    return;
  }
  if ( dimension < layout.dimensions.size() ) {
    size_t span = layout.stride;
    for ( size_t d = dimension + 1; d < layout.dimensions.size(); d++ ) {
      span *= layout.dimensions[d];
    }
    size_t i = 0;
    for ( auto& element : value ) {
      fillJson(layout, element, values, offset + i * span, dimension + 1, path);
      ++i;
    }
    return;
  }
  if ( layout.scalar.has_value() ) {
    values[offset] = jsonValue(value, layout.scalar.value(), path);
    return;
  }
  for ( auto& field : layout.fields ) {
    if ( auto it = value.find(field.name); it != value.end() ) {
      fillJson(field.layout, *it, values, offset + field.offset, 0, path + "." + field.name);
    }
  }
}

} // namespace

std::shared_ptr<const BPMNOS::Object> Schema::fromJSON(const std::string& text) const {
  Json root;
  try {
    root = Json::parse(text);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error(std::string("Schema: illegal JSON.\n") + error.what());
  }
  FlatSchema declared = flatten(*this);
  FlatSchema resolved = declared;
  resolveJson(resolved, { &root }, "");
  close(resolved);
  auto object = std::make_shared<BPMNOS::Object>();
  object->layout = std::make_shared<const BPMNOS::Object::Layout>( layoutOf(resolved, declared) );
  object->values.resize( object->layout->size() );
  fillJson(*object->layout, root, object->values, 0, 0, "");
  return object;
}
