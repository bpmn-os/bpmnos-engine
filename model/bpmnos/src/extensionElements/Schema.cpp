#include "Schema.h"
#include <cctype>
#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>

using namespace BPMNOS::Model;

namespace {

/**
 * @brief Recursive descent parser of the grammar `type := base dimension*`.
 */
class SchemaParser {
public:
  explicit SchemaParser(const std::string& text) : text(text), position(0) {}

  Schema parseType() {
    Schema schema;
    skipWhitespace();
    if ( peek() == '{' ) {
      parseObject(schema);
    }
    else {
      schema.scalar = parseScalar();
    }
    skipWhitespace();
    while ( peek() == '[' ) {
      ++position;
      skipWhitespace();
      if ( peek() == ']' ) {
        schema.dimensions.push_back(std::nullopt);
      }
      else {
        schema.dimensions.push_back(parseSize());
      }
      skipWhitespace();
      expect(']');
      skipWhitespace();
    }
    return schema;
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

  BPMNOS::ValueType parseScalar() {
    std::string name = parseName();
    if ( name == "boolean" ) return BPMNOS::ValueType::BOOLEAN;
    if ( name == "integer" ) return BPMNOS::ValueType::INTEGER;
    if ( name == "decimal" ) return BPMNOS::ValueType::DECIMAL;
    if ( name == "string" ) return BPMNOS::ValueType::STRING;
    if ( name == "collection" ) return BPMNOS::ValueType::COLLECTION;
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

  void parseObject(Schema& schema) {
    expect('{');
    skipWhitespace();
    if ( peek() == '}' ) {
      fail("an object requires at least one field");
    }
    while ( true ) {
      skipWhitespace();
      std::string name = parseName();
      for ( auto& [field, _] : schema.fields ) {
        if ( field == name ) {
          fail("duplicate field '" + name + "'");
        }
      }
      skipWhitespace();
      expect(':');
      schema.fields.emplace_back(name, parseType());
      skipWhitespace();
      if ( peek() == ',' ) {
        ++position;
        continue;
      }
      expect('}');
      return;
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
  std::string result;
  if ( scalar.has_value() ) {
    switch ( scalar.value() ) {
      case BPMNOS::ValueType::BOOLEAN: result = "boolean"; break;
      case BPMNOS::ValueType::INTEGER: result = "integer"; break;
      case BPMNOS::ValueType::DECIMAL: result = "decimal"; break;
      case BPMNOS::ValueType::STRING: result = "string"; break;
      case BPMNOS::ValueType::COLLECTION: result = "collection"; break;
    }
  }
  else {
    result = "{ ";
    for ( size_t i = 0; i < fields.size(); i++ ) {
      result += ( i ? ", " : "" ) + fields[i].first + ": " + fields[i].second.stringify();
    }
    result += " }";
  }
  for ( auto& dimension : dimensions ) {
    result += dimension.has_value() ? "[" + std::to_string(dimension.value()) + "]" : "[]";
  }
  return result;
}

void Schema::declareSizes(const std::string& steps) {
  Schema* node = this;
  size_t dimension = 0;
  size_t position = 0;
  auto fail = [&steps](const std::string& reason) {
    throw std::runtime_error("Schema: illegal size declaration '" + steps + "': " + reason);
  };
  auto skipWhitespace = [&]() {
    while ( position < steps.size() && std::isspace((unsigned char)steps[position]) ) {
      ++position;
    }
  };
  while ( true ) {
    skipWhitespace();
    if ( position == steps.size() ) {
      return;
    }
    if ( steps[position] == '[' ) {
      ++position;
      skipWhitespace();
      size_t start = position;
      while ( position < steps.size() && std::isdigit((unsigned char)steps[position]) ) {
        ++position;
      }
      std::optional<size_t> size;
      if ( position > start ) {
        size = std::stoul(steps.substr(start, position - start));
        if ( size.value() == 0 ) {
          fail("a size must be positive");
        }
      }
      skipWhitespace();
      if ( position == steps.size() || steps[position] != ']' ) {
        fail("expected ']'");
      }
      ++position;
      if ( dimension >= node->dimensions.size() ) {
        fail("more dimensions than the type has");
      }
      auto& declared = node->dimensions[dimension];
      if ( size.has_value() ) {
        if ( declared.has_value() && declared.value() != size.value() ) {
          fail("size " + std::to_string(size.value()) + " contradicts size " + std::to_string(declared.value()));
        }
        declared = size;
      }
      ++dimension;
    }
    else if ( steps[position] == '.' ) {
      ++position;
      size_t start = position;
      while ( position < steps.size() && ( std::isalnum((unsigned char)steps[position]) || steps[position] == '_' ) ) {
        ++position;
      }
      std::string name = steps.substr(start, position - start);
      auto field = std::find_if(node->fields.begin(), node->fields.end(), [&name](auto& candidate) { return candidate.first == name; });
      if ( field == node->fields.end() ) {
        fail("unknown field '" + name + "'");
      }
      node = &field->second;
      dimension = 0;
    }
    else {
      fail("unexpected '" + steps.substr(position) + "'");
    }
  }
}

bool Schema::isFixed() const {
  for ( auto& dimension : dimensions ) {
    if ( !dimension.has_value() ) {
      return false;
    }
  }
  for ( auto& [_, field] : fields ) {
    if ( !field.isFixed() ) {
      return false;
    }
  }
  return true;
}

namespace {

/// Returns the layout of a schema whose dimensions all have sizes, a dimension being fixed if it has a size in
/// the schema declared, of which the schema is a resolution.
BPMNOS::Object::Layout layoutOf(const Schema& schema, const Schema& declared) {
  BPMNOS::Object::Layout layout;
  layout.scalar = schema.scalar;
  for ( size_t k = 0; k < schema.fields.size(); k++ ) {
    layout.fields.push_back( BPMNOS::Object::Layout::Field{ schema.fields[k].first, 0, layoutOf(schema.fields[k].second, declared.fields[k].second) } );
  }
  for ( size_t d = 0; d < schema.dimensions.size(); d++ ) {
    assert( schema.dimensions[d].has_value() );
    layout.dimensions.push_back( schema.dimensions[d].value() );
    layout.fixed.push_back( declared.dimensions[d].has_value() );
  }
  layout.arrange();
  return layout;
}

/// Fixes the open dimensions of a schema by the layout of a constant object, checking that it fits.
void resolve(Schema& schema, const BPMNOS::Object::Layout& constant, const std::string& path) {
  if ( constant.dimensions.size() != schema.dimensions.size() ) {
    throw std::runtime_error("Schema: '" + path + "' has " + std::to_string(constant.dimensions.size()) + " dimensions instead of " + std::to_string(schema.dimensions.size()));
  }
  for ( size_t i = 0; i < schema.dimensions.size(); i++ ) {
    if ( !schema.dimensions[i].has_value() ) {
      schema.dimensions[i] = constant.dimensions[i];
    }
    else if ( constant.dimensions[i] > schema.dimensions[i].value() ) {
      throw std::runtime_error("Schema: '" + path + "' has " + std::to_string(constant.dimensions[i]) + " elements where " + std::to_string(schema.dimensions[i].value()) + " are declared");
    }
  }
  if ( schema.scalar.has_value() ) {
    if ( !constant.scalar.has_value() ) {
      throw std::runtime_error("Schema: '" + path + "' has fields where a value is expected");
    }
    auto from = constant.scalar.value();
    auto to = schema.scalar.value();
    bool numeric = ( from == BPMNOS::ValueType::DECIMAL || from == BPMNOS::ValueType::BOOLEAN );
    bool fits = ( from == to ) || ( numeric && ( to == BPMNOS::ValueType::DECIMAL || to == BPMNOS::ValueType::INTEGER || to == BPMNOS::ValueType::BOOLEAN ) );
    if ( !fits ) {
      throw std::runtime_error("Schema: '" + path + "' cannot be converted to the type of the attribute");
    }
    return;
  }
  if ( constant.scalar.has_value() ) {
    throw std::runtime_error("Schema: '" + path + "' is a value where fields are expected");
  }
  for ( auto& field : constant.fields ) {
    auto it = std::find_if(schema.fields.begin(), schema.fields.end(), [&field](auto& candidate) { return candidate.first == field.name; });
    if ( it == schema.fields.end() ) {
      throw std::runtime_error("Schema: '" + path + "' has the unknown field '" + field.name + "'");
    }
    resolve(it->second, field.layout, path + "." + field.name);
  }
}

/// Gives the dimensions still open, of a field the value lacks or below an empty array, the length zero, which
/// leaves them undetermined until the first assignment of a value to one of their elements.
void close(Schema& schema) {
  for ( auto& dimension : schema.dimensions ) {
    if ( !dimension.has_value() ) {
      dimension = 0;
    }
  }
  for ( auto& [name, field] : schema.fields ) {
    close(field);
  }
}

/// Converts a scalar value of a constant object to the type of the schema.
BPMNOS::Value convert(const BPMNOS::Value& value, BPMNOS::ValueType type) {
  if ( !value.has_value() ) {
    return std::nullopt;
  }
  switch ( type ) {
    case BPMNOS::ValueType::INTEGER:
      return BPMNOS::number((long)(double)value.value());
    case BPMNOS::ValueType::BOOLEAN:
      return BPMNOS::number( value.value() != BPMNOS::number(0) ? 1 : 0 );
    default:
      return value;
  }
}

/// Copies the values of a constant object of the given layout into an object of the target layout.
void copyValues(const BPMNOS::Object::Layout& target, const BPMNOS::Object::Layout& source, const std::vector<BPMNOS::Value>& from, size_t fromOffset, std::vector<BPMNOS::Value>& to, size_t toOffset) {
  std::function<void(size_t, size_t, size_t)> copyDimension = [&](size_t dimension, size_t sourceOffset, size_t targetOffset) {
    if ( dimension == source.dimensions.size() ) {
      if ( target.scalar.has_value() ) {
        to[targetOffset] = convert(from[sourceOffset], target.scalar.value());
        return;
      }
      for ( auto& field : source.fields ) {
        auto it = std::find_if(target.fields.begin(), target.fields.end(), [&field](auto& candidate) { return candidate.name == field.name; });
        copyValues(it->layout, field.layout, from, sourceOffset + field.offset, to, targetOffset + it->offset);
      }
      return;
    }
    size_t sourceSpan = source.stride;
    size_t targetSpan = target.stride;
    for ( size_t d = dimension + 1; d < source.dimensions.size(); d++ ) {
      sourceSpan *= source.dimensions[d];
      targetSpan *= target.dimensions[d];
    }
    for ( size_t i = 0; i < source.dimensions[dimension]; i++ ) {
      copyDimension(dimension + 1, sourceOffset + i * sourceSpan, targetOffset + i * targetSpan);
    }
  };
  copyDimension(0, fromOffset, toOffset);
}

} // namespace

std::shared_ptr<const BPMNOS::Object> Schema::undefinedObject() const {
  // an object is created modifiable, so that a write may change it in place once it is no longer shared
  auto object = std::make_shared<BPMNOS::Object>();
  object->layout = std::make_shared<const BPMNOS::Object::Layout>( layoutOf(*this, *this) );
  object->values.resize( object->layout->size() );
  return object;
}

std::shared_ptr<const BPMNOS::Object> Schema::conform(const std::shared_ptr<const BPMNOS::Object>& constant) const {
  Schema resolved = *this;
  resolve(resolved, *constant->layout, BPMNOS::to_string(*constant));
  close(resolved);
  auto layout = layoutOf(resolved, *this);
  if ( layout == *constant->layout ) {
    // the object holds exactly what the attribute declares, with the same dimensions fixed, and is shared
    return constant;
  }
  // an object is created modifiable, so that a write may change it in place once it is no longer shared
  auto object = std::make_shared<BPMNOS::Object>();
  object->layout = std::make_shared<const BPMNOS::Object::Layout>( std::move(layout) );
  object->values.resize( object->layout->size() );
  copyValues(*object->layout, *constant->layout, constant->values, 0, object->values, 0);
  return object;
}

Schema Schema::of(const BPMNOS::Object::Layout& layout, bool element) {
  Schema schema;
  schema.scalar = layout.scalar;
  for ( auto& field : layout.fields ) {
    schema.fields.emplace_back( field.name, of(field.layout, element) );
  }
  for ( size_t d = 0; d < layout.dimensions.size(); d++ ) {
    bool fixed = d < layout.fixed.size() && layout.fixed[d];
    // an element keeps every length but one still undetermined, whereas an object replaced keeps only the
    // lengths that are fixed
    bool keep = element ? ( fixed || layout.dimensions[d] > 0 ) : fixed;
    schema.dimensions.push_back( keep ? std::optional<size_t>(layout.dimensions[d]) : std::nullopt );
  }
  return schema;
}
