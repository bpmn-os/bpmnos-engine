#include "Schema.h"
#include <cctype>
#include <algorithm>
#include <cassert>
#include <functional>
#include <stdexcept>
#include <variant>

using namespace BPMNOS;

namespace {

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
    return *valueType == std::get<ValueType>(source);
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

/// Returns the target schema for the effective schema and the current layout.
Schema determineTargetSchema(const Schema& effectiveSchema, const BPMNOS::Layout& currentLayout) {
  if ( auto array = std::get_if<ArrayType>(&effectiveSchema) ) {
    auto& current = std::get<BPMNOS::ArrayLayout>(currentLayout);
    auto size = array->size.has_value() ? std::optional<size_t>(current.size) : std::nullopt;
    return Schema( ArrayType{ std::make_shared<const Schema>( determineTargetSchema(*array->type, *current.layout) ), size } );
  }
  if ( std::holds_alternative<ValueType>(effectiveSchema) ) {
    return effectiveSchema;
  }
  auto& fields = std::get<ObjectType>(effectiveSchema).fields;
  auto& current = std::get<BPMNOS::ObjectLayout>(currentLayout).fields;
  assert( fields.size() == current.size() );
  ObjectType object;
  for ( size_t k = 0; k < fields.size(); k++ ) {
    object.fields.push_back( Field{ fields[k].name, std::make_shared<const Schema>( determineTargetSchema(*fields[k].type, *current[k].layout) ) } );
  }
  return Schema( std::move(object) );
}

} // namespace

Schema::Schema(const Schema& effectiveSchema, const BPMNOS::Layout& currentLayout)
  : Schema( determineTargetSchema(effectiveSchema, currentLayout) )
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
