#include "Object.h"
#include "Value.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <stdexcept>

namespace BPMNOS {

size_t Object::Layout::size() const {
  size_t size = stride;
  for ( auto dimension : dimensions ) {
    size *= dimension;
  }
  return size;
}

std::string Object::Layout::stringify() const {
  std::string result;
  if ( scalar.has_value() ) {
    switch ( scalar.value() ) {
      case ValueType::BOOLEAN: result = "boolean"; break;
      case ValueType::INTEGER: result = "integer"; break;
      case ValueType::DECIMAL: result = "decimal"; break;
      case ValueType::STRING: result = "string"; break;
      case ValueType::COLLECTION: result = "collection"; break;
    }
  }
  else {
    result = "{ ";
    for ( size_t i = 0; i < fields.size(); i++ ) {
      result += ( i ? ", " : "" ) + fields[i].name + ": " + fields[i].layout.stringify();
    }
    result += " }";
  }
  for ( auto dimension : dimensions ) {
    result += "[" + std::to_string(dimension) + "]";
  }
  return result;
}

void Object::Layout::arrange() {
  if ( scalar.has_value() ) {
    stride = 1;
    return;
  }
  size_t offset = 0;
  for ( auto& field : fields ) {
    field.offset = offset;
    offset += field.layout.size();
  }
  stride = offset;
}

bool Object::Layout::operator==(const Layout& other) const {
  return scalar == other.scalar && fields == other.fields && dimensions == other.dimensions && stride == other.stride && fixed == other.fixed;
}

namespace {

/// Renders the values of a layout beginning at the given offset, from the given dimension on.
std::string render(const Object::Layout& layout, const std::vector<Value>& values, size_t offset, size_t dimension) {
  if ( dimension < layout.dimensions.size() ) {
    // the elements along this dimension, each holding the remaining dimensions
    size_t span = layout.stride;
    for ( size_t d = dimension + 1; d < layout.dimensions.size(); d++ ) {
      span *= layout.dimensions[d];
    }
    if ( layout.dimensions[dimension] == 0 ) {
      return "[ ]";
    }
    std::string result;
    for ( size_t i = 0; i < layout.dimensions[dimension]; i++ ) {
      result += ", " + render(layout, values, offset + i * span, dimension + 1);
    }
    result.front() = '[';
    return result + " ]";
  }
  if ( layout.scalar.has_value() ) {
    auto& value = values[offset];
    if ( !value.has_value() ) {
      return "undefined";
    }
    auto text = BPMNOS::to_string(value.value(), layout.scalar.value());
    return layout.scalar.value() == ValueType::STRING ? "\"" + text + "\"" : text;
  }
  std::string result = "{ ";
  for ( size_t i = 0; i < layout.fields.size(); i++ ) {
    auto& field = layout.fields[i];
    result += ( i ? ", " : "" ) + field.name + " := " + render(field.layout, values, offset + field.offset, 0);
  }
  return result + " }";
}

} // namespace

namespace {

/// Returns the number of values one element of the given dimension of a layout spans.
size_t span(const Object::Layout& layout, size_t dimension) {
  size_t result = layout.stride;
  for ( size_t d = dimension + 1; d < layout.dimensions.size(); d++ ) {
    result *= layout.dimensions[d];
  }
  return result;
}

/**
 * Visits the values of an object of the layout `from` that an object of the layout `to`, of the same schema,
 * also has, calling `visit(source, target)` with the positions of each such value in the two objects.
 */
void correspond(const Object::Layout& from, size_t fromOffset, const Object::Layout& to, size_t toOffset, const std::function<void(size_t, size_t)>& visit) {
  std::function<void(size_t, size_t, size_t)> walk = [&](size_t dimension, size_t source, size_t target) {
    if ( dimension == from.dimensions.size() ) {
      if ( from.scalar.has_value() ) {
        visit(source, target);
        return;
      }
      for ( size_t k = 0; k < from.fields.size(); k++ ) {
        correspond(from.fields[k].layout, source + from.fields[k].offset, to.fields[k].layout, target + to.fields[k].offset, visit);
      }
      return;
    }
    size_t count = std::min(from.dimensions[dimension], to.dimensions[dimension]);
    size_t fromSpan = span(from, dimension);
    size_t toSpan = span(to, dimension);
    for ( size_t i = 0; i < count; i++ ) {
      walk(dimension + 1, source + i * fromSpan, target + i * toSpan);
    }
  };
  walk(0, fromOffset, toOffset);
}

/// Widens a layout so that every dimension has at least the length it has in the other layout.
void widen(Object::Layout& layout, const Object::Layout& other) {
  for ( size_t d = 0; d < layout.dimensions.size(); d++ ) {
    layout.dimensions[d] = std::max(layout.dimensions[d], other.dimensions[d]);
  }
  for ( size_t k = 0; k < layout.fields.size(); k++ ) {
    widen(layout.fields[k].layout, other.fields[k].layout);
  }
  layout.arrange();
}

} // namespace

Object& modifiable(std::shared_ptr<const Object>& slot) {
  if ( slot.use_count() != 1 ) {
    slot = std::make_shared<Object>(*slot);
  }
  // an object held by the slot alone was created modifiable, a constant object being held by the registry too
  return const_cast<Object&>(*slot);
}

std::shared_ptr<Object> resized(const Object& object, const std::vector<std::string>& fields, size_t dimension, size_t length) {
  auto layout = std::make_shared<Object::Layout>(*object.layout);
  // the layouts from the base to the field whose dimension changes
  std::vector<Object::Layout*> chain = { layout.get() };
  for ( auto& name : fields ) {
    auto& candidates = chain.back()->fields;
    auto field = std::ranges::find_if(candidates, [&name](auto& candidate) { return candidate.name == name; });
    if ( field == candidates.end() ) {
      throw std::logic_error("Object: unknown field '" + name + "'");
    }
    chain.push_back(&field->layout);
  }
  chain.back()->dimensions.at(dimension) = length;
  for ( auto it = chain.rbegin(); it != chain.rend(); ++it ) {
    (*it)->arrange();
  }
  auto result = std::make_shared<Object>();
  result->values.resize(layout->size());
  correspond(*object.layout, 0, *layout, 0, [&](size_t source, size_t target) {
    result->values[target] = object.values[source];
  });
  result->layout = std::move(layout);
  return result;
}

std::shared_ptr<const Object> merge(const std::vector< std::shared_ptr<const Object> >& objects) {
  assert( !objects.empty() );
  if ( std::ranges::all_of(objects, [&objects](auto& object) { return object == objects.front(); }) ) {
    return objects.front();
  }
  auto layout = std::make_shared<Object::Layout>(*objects.front()->layout);
  for ( auto& object : objects ) {
    widen(*layout, *object->layout);
  }
  auto result = std::make_shared<Object>();
  result->values.resize(layout->size());
  // a value is merged as a status value is: the first defined value is kept, and a conflict with it makes the
  // value undefined for good
  std::vector<bool> conflict(layout->size(), false);
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
  return render(*object.layout, object.values, 0, 0);
}

std::vector<double> to_vector(const Object& object) {
  if ( !object.isVector() ) {
    throw std::runtime_error("Object: '" + to_string(object) + "' is not an array of values");
  }
  std::vector<double> result;
  result.reserve(object.values.size());
  for ( auto& value : object.values ) {
    result.push_back( value.has_value() ? (double)value.value() : std::numeric_limits<double>::quiet_NaN() );
  }
  return result;
}

} // namespace BPMNOS
