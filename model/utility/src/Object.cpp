#include "Object.h"
#include "Value.h"
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

bool Object::Layout::operator==(const Layout& other) const {
  return scalar == other.scalar && fields == other.fields && dimensions == other.dimensions && stride == other.stride;
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
