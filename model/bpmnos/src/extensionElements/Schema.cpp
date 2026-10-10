#include "Schema.h"
#include <cctype>
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
