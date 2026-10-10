#include "InputEncoder.h"

#include <cctype>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ObjectRegistry.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include "string_utility.h"

using namespace BPMNOS;

namespace {

/// Whether the character may occur in a name.
bool isNameCharacter(char character) {
  return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
}

/// Whether the character separates what surrounds it.
bool isSpace(char character) {
  return std::isspace(static_cast<unsigned char>(character)) != 0;
}

/// The membership operators written as symbols. A bracketed group following one of them states a
/// collection of the expression parser's own, which reads it that way itself, so the scan copies such a
/// group rather than registering it. The same operators written as the names `in` and `not in` are read
/// as names and their group is copied for that reason already.
const std::string membershipOperators[] = { "∈", "∉" };

/**
 * @brief A literal as it is read, before it is registered as a constant object.
 */
struct Literal {
  enum class Kind { SCALAR, ARRAY, OBJECT };
  Kind kind = Kind::SCALAR;
  double value = 0; ///< The value of a scalar, a string being its index in the string registry
  ValueType type = DECIMAL; ///< The type of a scalar
  std::vector<Literal> elements; ///< The elements of an array
  std::vector< std::pair<std::string, Literal> > fields; ///< The fields of a value with fields, in their order
};

/**
 * @brief The scan of one text, holding where it has got to and what it has emitted.
 */
class Scan {
public:
  Scan(const std::string& text) : input(text) {}

  /// Reads the text to its end.
  void run();

  /// Returns what the scan emitted.
  std::string result() { return std::move(output); }

  /// Returns the type of the literal if the text states one literal and nothing besides whitespace.
  std::optional<ValueType> type() const;

  /// Returns the indices of the constant objects the literals of the text state, in their order.
  const std::vector<size_t>& objects() const { return objectIndices; }

private:
  /// Reads a quoted span and returns its index in the string registry.
  size_t scanString();
  /// Reads a literal at the current position, `[ ... ]` or `{ name := ... }`, and returns the index of the
  /// constant object it states in the object registry.
  size_t scanObject();
  /// Reads an array literal.
  Literal scanArray();
  /// Reads a literal with fields.
  Literal scanFields();
  /// Reads one member of an array or the value of a field, ending before the given closing character.
  Literal scanMember(char closing);
  /// Returns the layout of a literal, refusing elements of an array that differ in layout.
  Object::Layout layoutOf(const Literal& literal) const;
  /// Writes the values of a literal into the values of an object, beginning at the given offset.
  void flatten(const Literal& literal, std::vector<Value>& values, size_t offset) const;
  /// Returns true if the text at the current position opens a literal with fields, `{ name := ...`, rather than
  /// a set or the body of an aggregation.
  bool fieldsAhead() const;
  /// Notes that a literal of the given type was read from the given position to the current one.
  void recordLiteral(size_t begin, ValueType type);
  /// Returns the membership operator at the current position, and an empty string where there is none.
  std::string scanMembershipOperator() const;
  void skipSpace();
  [[noreturn]] void fail(const std::string& reason) const;

  const std::string& input;
  size_t position = 0;
  std::string output;
  /// Whether a bracket met here opens a literal, which it does not where it indexes a collection or
  /// states a collection of the expression parser's own.
  bool literalAllowed = true;
  size_t literals = 0;
  std::vector<size_t> objectIndices;
  size_t literalBegin = 0;
  size_t literalEnd = 0;
  ValueType literalType = COLLECTION;
};

void Scan::run() {
  while ( position < input.size() ) {
    char character = input[position];

    if ( character == '"' ) {
      size_t begin = position;
      output += std::to_string( scanString() );
      recordLiteral(begin, STRING);
      literalAllowed = false;
    }
    else if ( ( character == '[' || ( character == '{' && fieldsAhead() ) ) && literalAllowed ) {
      size_t begin = position;
      objectIndices.push_back( scanObject() );
      output += std::to_string( objectIndices.back() );
      recordLiteral(begin, COLLECTION);
      literalAllowed = false;
    }
    else if ( character == '[' || character == '(' ) {
      // an index or a group opens, and what follows it stands on its own
      output += character;
      ++position;
      literalAllowed = true;
    }
    else if ( character == ']' || character == ')' ) {
      output += character;
      ++position;
      literalAllowed = false;
    }
    else if ( isNameCharacter(character) ) {
      while ( position < input.size() && isNameCharacter(input[position]) ) {
        output += input[position];
        ++position;
      }
      literalAllowed = false;
    }
    else if ( isSpace(character) ) {
      // whitespace neither opens nor closes anything
      output += character;
      ++position;
    }
    else if ( auto operator_ = scanMembershipOperator(); !operator_.empty() ) {
      output += operator_;
      position += operator_.size();
      literalAllowed = false;
    }
    else {
      output += character;
      ++position;
      literalAllowed = true;
    }
  }
}

std::optional<ValueType> Scan::type() const {
  if ( literals != 1 ) {
    return std::nullopt;
  }

  for ( size_t i = 0; i < literalBegin; i++ ) {
    if ( !isSpace(input[i]) ) {
      return std::nullopt;
    }
  }
  for ( size_t i = literalEnd; i < input.size(); i++ ) {
    if ( !isSpace(input[i]) ) {
      return std::nullopt;
    }
  }

  return literalType;
}

size_t Scan::scanString() {
  size_t begin = position + 1; // skip the opening quote
  size_t end = input.find('"', begin);

  if ( end == std::string::npos ) {
    fail("unterminated string");
  }

  position = end + 1;
  return stringRegistry( input.substr(begin, end - begin) );
}

bool Scan::fieldsAhead() const {
  size_t ahead = position + 1;
  while ( ahead < input.size() && isSpace(input[ahead]) ) {
    ++ahead;
  }
  if ( ahead >= input.size() || !( std::isalpha(static_cast<unsigned char>(input[ahead])) || input[ahead] == '_' ) ) {
    return false;
  }
  while ( ahead < input.size() && isNameCharacter(input[ahead]) ) {
    ++ahead;
  }
  while ( ahead < input.size() && isSpace(input[ahead]) ) {
    ++ahead;
  }
  return input.compare(ahead, 2, ":=") == 0;
}

size_t Scan::scanObject() {
  Literal literal = ( input[position] == '[' ? scanArray() : scanFields() );
  Object object;
  object.layout = std::make_shared<const Object::Layout>( layoutOf(literal) );
  object.values.resize( object.layout->size() );
  flatten(literal, object.values, 0);
  return objectRegistry( std::move(object) );
}

Literal Scan::scanArray() {
  ++position; // skip the opening bracket
  Literal literal;
  literal.kind = Literal::Kind::ARRAY;

  skipSpace();
  if ( position < input.size() && input[position] == ']' ) {
    fail("array without members");
  }

  while ( true ) {
    skipSpace();
    literal.elements.push_back( scanMember(']') );

    skipSpace();
    if ( position >= input.size() ) {
      fail("unterminated array");
    }
    if ( input[position] == ',' ) {
      ++position;
      continue;
    }
    if ( input[position] == ']' ) {
      ++position;
      break;
    }
    fail("illegal member");
  }
  return literal;
}

Literal Scan::scanFields() {
  ++position; // skip the opening brace
  Literal literal;
  literal.kind = Literal::Kind::OBJECT;

  while ( true ) {
    skipSpace();
    size_t begin = position;
    while ( position < input.size() && isNameCharacter(input[position]) ) {
      ++position;
    }
    std::string name = input.substr(begin, position - begin);
    if ( name.empty() ) {
      fail("field without name");
    }
    for ( auto& [field, _] : literal.fields ) {
      if ( field == name ) {
        fail("duplicate field '" + name + "'");
      }
    }
    skipSpace();
    if ( input.compare(position, 2, ":=") != 0 ) {
      fail("field '" + name + "' without ':='");
    }
    position += 2;
    skipSpace();
    literal.fields.emplace_back( name, scanMember('}') );

    skipSpace();
    if ( position >= input.size() ) {
      fail("unterminated value with fields");
    }
    if ( input[position] == ',' ) {
      ++position;
      continue;
    }
    if ( input[position] == '}' ) {
      ++position;
      break;
    }
    fail("illegal field");
  }
  return literal;
}

Literal Scan::scanMember(char closing) {
  if ( position >= input.size() ) {
    fail("unterminated literal");
  }

  Literal literal;
  if ( input[position] == '"' ) {
    literal.value = static_cast<double>( scanString() );
    literal.type = STRING;
    return literal;
  }
  if ( input[position] == '[' ) {
    return scanArray();
  }
  if ( input[position] == '{' ) {
    return scanFields();
  }

  size_t begin = position;
  while ( position < input.size() && input[position] != ',' && input[position] != closing ) {
    ++position;
  }

  std::string member = BPMNOS::trim_copy( input.substr(begin, position - begin) );

  if ( member.empty() ) {
    fail("member without value");
  }
  if ( member == Keyword::False ) {
    literal.value = 0.0;
    literal.type = BOOLEAN;
    return literal;
  }
  if ( member == Keyword::True ) {
    literal.value = 1.0;
    literal.type = BOOLEAN;
    return literal;
  }

  // a number is recorded as a decimal, so that whole and fractional members agree in type
  size_t consumed = 0;
  try {
    literal.value = std::stod(member, &consumed);
  }
  catch ( const std::exception& ) {
    fail("illegal member '" + member + "'");
  }
  if ( consumed != member.size() ) {
    fail("illegal member '" + member + "'");
  }
  literal.type = DECIMAL;
  return literal;
}

Object::Layout Scan::layoutOf(const Literal& literal) const {
  Object::Layout layout;
  switch ( literal.kind ) {
    case Literal::Kind::SCALAR:
      layout.scalar = literal.type;
      return layout;
    case Literal::Kind::ARRAY:
    {
      // the elements of an array are uniform: they agree in type and in the sizes of their own arrays
      layout = layoutOf(literal.elements.front());
      for ( auto& element : literal.elements ) {
        if ( !( layoutOf(element) == layout ) ) {
          fail("members of different type or size");
        }
      }
      layout.dimensions.insert(layout.dimensions.begin(), literal.elements.size());
      return layout;
    }
    case Literal::Kind::OBJECT:
    {
      size_t offset = 0;
      for ( auto& [name, value] : literal.fields ) {
        auto fieldLayout = layoutOf(value);
        auto size = fieldLayout.size();
        layout.fields.push_back( Object::Layout::Field{ name, offset, std::move(fieldLayout) } );
        offset += size;
      }
      layout.stride = offset;
      return layout;
    }
  }
  throw std::logic_error("InputEncoder: unknown kind of literal");
}

void Scan::flatten(const Literal& literal, std::vector<Value>& values, size_t offset) const {
  switch ( literal.kind ) {
    case Literal::Kind::SCALAR:
      values[offset] = number(literal.value);
      return;
    case Literal::Kind::ARRAY:
    {
      size_t span = layoutOf(literal.elements.front()).size();
      for ( size_t i = 0; i < literal.elements.size(); i++ ) {
        flatten(literal.elements[i], values, offset + i * span);
      }
      return;
    }
    case Literal::Kind::OBJECT:
    {
      size_t fieldOffset = 0;
      for ( auto& [name, value] : literal.fields ) {
        flatten(value, values, offset + fieldOffset);
        fieldOffset += layoutOf(value).size();
      }
      return;
    }
  }
}

std::string Scan::scanMembershipOperator() const {
  for ( auto& operator_ : membershipOperators ) {
    if ( input.compare(position, operator_.size(), operator_) == 0 ) {
      return operator_;
    }
  }
  return {};
}

void Scan::recordLiteral(size_t begin, ValueType type) {
  literals++;
  literalBegin = begin;
  literalEnd = position;
  literalType = type;
}

void Scan::skipSpace() {
  while ( position < input.size() && isSpace(input[position]) ) {
    ++position;
  }
}

void Scan::fail(const std::string& reason) const {
  throw std::runtime_error("InputEncoder: " + reason + " in '" + input + "'");
}

} // namespace

InputEncoder::InputEncoder(const std::string& input) {
  Scan scan(input);
  scan.run();
  encoded = scan.result();
  literalType = scan.type();
  literalObjects = scan.objects();
}

InputEncoder::InputEncoder(std::string text, std::nullopt_t)
  : encoded(std::move(text))
  , literalType(std::nullopt)
{
}

InputEncoder InputEncoder::fragment(std::string text) {
  return InputEncoder(std::move(text), std::nullopt);
}
