#include "InputEncoder.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "ObjectRegistry.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include "string_utility.h"

using namespace BPMNOS;

/// A single value: a number, an integer or a decimal as it is written, a truth value, or a string, held as its index
/// in the string registry.
struct BPMNOS::InputEncoder::Scalar {
  double value;
  ValueType type;
};

/// An expression member of an object initializer, which has a value only once it is evaluated.
struct BPMNOS::InputEncoder::Expression {
  std::string text; ///< The text of the expression, scanned already
};

/// An array, whose elements are uniform.
struct BPMNOS::InputEncoder::Array {
  std::vector<Literal> elements;
};

/// A value with fields, in the order in which they are written.
struct BPMNOS::InputEncoder::Fields {
  std::vector< std::pair<std::string, Literal> > members;
};

/// A literal as it is read, before it is registered as a constant object or instantiated.
struct BPMNOS::InputEncoder::Literal {
  std::variant<Scalar, Expression, Array, Fields> node;
};

namespace {

using Literal = InputEncoder::Literal;
using Scalar = InputEncoder::Scalar;
using Expression = InputEncoder::Expression;
using Array = InputEncoder::Array;
using Fields = InputEncoder::Fields;

/// Visitor calling the overload of the alternative visited.
template<class... Visitors> struct Overloaded : Visitors... { using Visitors::operator()...; };

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

/// Returns true if a literal has an expression member, at any depth.
bool containsExpression(const Literal& literal) {
  return std::visit(Overloaded{
    [](const Scalar&) { return false; },
    [](const Expression&) { return true; },
    [](const Array& array) {
      return std::ranges::any_of(array.elements, [](auto& element) { return containsExpression(element); });
    },
    [](const Fields& fields) {
      return std::ranges::any_of(fields.members, [](auto& member) { return containsExpression(member.second); });
    }
  }, literal.node);
}

/**
 * @brief The scan of one text, holding where it has got to and what it has emitted.
 */
class Scan {
public:
  /// Constructor for the scan of a text, admitting object initializers if requested.
  Scan(const std::string& text, bool initializers = false) : input(text), initializers(initializers) {}

  /// Reads the text to its end.
  void run();

  /// Returns what the scan emitted.
  std::string result() { return std::move(output); }

  /// Returns the index of the constant object if the text states one literal array or value with fields and
  /// nothing besides whitespace.
  std::optional<size_t> getObjectIndex() const;

  /// Returns the indices of the constant objects the literals of the text state, in their order.
  const std::vector<size_t>& getObjectIndices() const { return objectIndices; }

  /// Returns where in what the scan emitted each literal stands.
  const std::vector<InputEncoder::EncodedLiteral>& getEncodedLiterals() const { return encodedLiterals; }

  /// Reads a text stating one literal and nothing besides whitespace.
  Literal scanInitializer();
  /// Returns the layout of a literal without expression members, refusing elements of an array that differ in
  /// layout.
  Object::Layout layoutOf(const Literal& literal) const;
  /// Returns the schema of a literal without expression members, which states its value types and leaves every
  /// array open, the elements of an array being uniform.
  Schema schemaOf(const Literal& literal) const;
  /// Writes the values of a literal without expression members into the values of an object, beginning at the
  /// given offset.
  void flatten(const Literal& literal, std::vector<Value>& values, size_t offset) const;

private:
  /// Reads a quoted span and returns its index in the string registry.
  size_t scanString();
  /// Reads a literal at the current position, `[ ... ]` or `{ name := ... }`.
  Literal scanLiteral();
  /// Registers a literal without expression members as a constant object and returns its index in the object
  /// registry.
  size_t registerObject(const Literal& literal) const;
  /// Reads an array literal.
  Literal scanArray();
  /// Reads a literal with fields.
  Literal scanFields();
  /// Reads one member of an array or the value of a field, ending before the given closing character.
  Literal scanMember(char closing);
  /// Returns the position at which a member beginning at the current position ends: the next comma, closing
  /// bracket or closing brace outside every parenthesis, bracket, brace and quote, or the end of the text.
  size_t memberEnd() const;
  /// Returns true if the text at the current position opens a literal with fields, `{ name := ...`, rather than
  /// a set or the body of an aggregation.
  bool fieldsAhead() const;
  /// Notes that a literal, an array or value with fields or else a quoted string, was read from the given
  /// position to the current one.
  void recordLiteral(size_t begin, bool object);
  /// Returns the membership operator at the current position, and an empty string where there is none.
  std::string scanMembershipOperator() const;
  void skipSpace();
  [[noreturn]] void fail(const std::string& reason) const;

  const std::string& input;
  const bool initializers; ///< Whether a literal may have expression members
  size_t position = 0;
  std::string output;
  /// Whether a bracket met here opens a literal, which it does not where it indexes a collection or
  /// states a collection of the expression parser's own.
  bool literalAllowed = true;
  size_t literals = 0;
  std::vector<size_t> objectIndices;
  std::vector<InputEncoder::EncodedLiteral> encodedLiterals;
  size_t literalBegin = 0;
  size_t literalEnd = 0;
  bool literalObject = false;
};

void Scan::run() {
  while ( position < input.size() ) {
    char character = input[position];

    if ( character == '"' ) {
      size_t begin = position;
      size_t emitted = output.size();
      output += std::to_string( scanString() );
      encodedLiterals.push_back( InputEncoder::EncodedLiteral{ emitted, output.size(), InputEncoder::EncodedLiteral::Type::STRING } );
      recordLiteral(begin, false);
      literalAllowed = false;
    }
    else if ( ( character == '[' || ( character == '{' && fieldsAhead() ) ) && literalAllowed ) {
      size_t begin = position;
      size_t emitted = output.size();
      Literal literal = scanLiteral();
      if ( containsExpression(literal) ) {
        // an object initializer is copied unchanged, to be read where it initialises an object
        output += input.substr(begin, position - begin);
        encodedLiterals.push_back( InputEncoder::EncodedLiteral{ emitted, output.size(), InputEncoder::EncodedLiteral::Type::INITIALIZER } );
        recordLiteral(begin, false);
      }
      else {
        objectIndices.push_back( registerObject(literal) );
        output += std::to_string( objectIndices.back() );
        encodedLiterals.push_back( InputEncoder::EncodedLiteral{ emitted, output.size(), InputEncoder::EncodedLiteral::Type::OBJECT } );
        recordLiteral(begin, true);
      }
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

std::optional<size_t> Scan::getObjectIndex() const {
  if ( literals != 1 || !literalObject ) {
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

  return objectIndices.back();
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

Literal Scan::scanLiteral() {
  return ( input[position] == '[' ? scanArray() : scanFields() );
}

Literal Scan::scanInitializer() {
  skipSpace();
  if ( position >= input.size() || !( input[position] == '[' || ( input[position] == '{' && fieldsAhead() ) ) ) {
    fail("no literal");
  }
  Literal literal = scanLiteral();
  skipSpace();
  if ( position < input.size() ) {
    fail("text besides the literal");
  }
  return literal;
}

size_t Scan::registerObject(const Literal& literal) const {
  Object object;
  object.layout = std::make_shared<const Object::Layout>( layoutOf(literal) );
  object.values.resize( object.layout->size() );
  flatten(literal, object.values, 0);
  return objectRegistry( std::move(object), schemaOf(literal) );
}

Literal Scan::scanArray() {
  ++position; // skip the opening bracket
  Array array;

  skipSpace();
  if ( position < input.size() && input[position] == ']' ) {
    fail("array without members");
  }

  while ( true ) {
    skipSpace();
    array.elements.push_back( scanMember(']') );

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
  return Literal{ std::move(array) };
}

Literal Scan::scanFields() {
  ++position; // skip the opening brace
  Fields fields;

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
    for ( auto& [field, _] : fields.members ) {
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
    fields.members.emplace_back( name, scanMember('}') );

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
  return Literal{ std::move(fields) };
}

Literal Scan::scanMember(char closing) {
  if ( position >= input.size() ) {
    fail("unterminated literal");
  }
  if ( input[position] == '[' ) {
    return scanArray();
  }
  if ( input[position] == '{' && fieldsAhead() ) {
    return scanFields();
  }

  size_t begin = position;
  position = memberEnd();
  if ( position >= input.size() ) {
    fail("unterminated literal");
  }
  if ( input[position] != ',' && input[position] != closing ) {
    fail("illegal member '" + input.substr(begin, position - begin + 1) + "'");
  }
  std::string member = BPMNOS::trim_copy( input.substr(begin, position - begin) );

  if ( member.empty() ) {
    fail("member without value");
  }

  if ( member.front() == '"' && member.find('"', 1) == member.size() - 1 ) {
    return Literal{ Scalar{ static_cast<double>( stringRegistry( member.substr(1, member.size() - 2) ) ), STRING } };
  }
  if ( member == Keyword::False ) {
    return Literal{ Scalar{ 0.0, BOOLEAN } };
  }
  if ( member == Keyword::True ) {
    return Literal{ Scalar{ 1.0, BOOLEAN } };
  }

  // a number is an integer if it is written without a decimal point and a decimal if it is written with one, and has
  // digits before and after the decimal point
  auto digits = [](std::string_view text) {
    return !text.empty() && std::ranges::all_of(text, [](char character) { return std::isdigit(static_cast<unsigned char>(character)) != 0; });
  };
  std::string_view number(member);
  if ( number.front() == '-' ) {
    number.remove_prefix(1);
  }
  auto point = number.find('.');
  if ( point == std::string_view::npos ? digits(number) : ( digits(number.substr(0, point)) && digits(number.substr(point + 1)) ) ) {
    return Literal{ Scalar{ std::stod(member), point == std::string_view::npos ? INTEGER : DECIMAL } };
  }

  if ( !initializers ) {
    fail("illegal member '" + member + "'");
  }
  // any other member is an expression, whose text is scanned as the text of any expression
  Scan scan(member);
  scan.run();
  return Literal{ Expression{ scan.result() } };
}

size_t Scan::memberEnd() const {
  size_t end = position;
  size_t depth = 0;
  while ( end < input.size() ) {
    char character = input[end];
    if ( character == '"' ) {
      end = input.find('"', end + 1);
      if ( end == std::string::npos ) {
        fail("unterminated string");
      }
    }
    else if ( character == '(' || character == '[' || character == '{' ) {
      depth++;
    }
    else if ( character == ')' || character == ']' || character == '}' ) {
      if ( depth == 0 ) {
        return end;
      }
      depth--;
    }
    else if ( character == ',' && depth == 0 ) {
      return end;
    }
    end++;
  }
  return end;
}

Object::Layout Scan::layoutOf(const Literal& literal) const {
  return std::visit(Overloaded{
    [](const Scalar& scalar) {
      Object::Layout layout;
      layout.scalar = scalar.type;
      return layout;
    },
    [this](const Expression&) -> Object::Layout {
      // an expression member has a layout only once it has a value
      fail("expression member without a value");
    },
    [this](const Array& array) {
      // the elements of an array are uniform: they agree in type and in the sizes of their own arrays
      auto layout = layoutOf(array.elements.front());
      for ( auto& element : array.elements ) {
        if ( !( layoutOf(element) == layout ) ) {
          fail("members of different type or size");
        }
      }
      layout.dimensions.insert(layout.dimensions.begin(), array.elements.size());
      return layout;
    },
    [this](const Fields& fields) {
      Object::Layout layout;
      size_t offset = 0;
      for ( auto& [name, member] : fields.members ) {
        auto fieldLayout = layoutOf(member);
        auto size = fieldLayout.size();
        layout.fields.push_back( Object::Layout::Field{ name, offset, std::move(fieldLayout) } );
        offset += size;
      }
      layout.stride = offset;
      return layout;
    }
  }, literal.node);
}

Schema Scan::schemaOf(const Literal& literal) const {
  return std::visit(Overloaded{
    [](const Scalar& scalar) {
      return Schema(scalar.type);
    },
    [this](const Expression&) -> Schema {
      // an expression member has a schema only once it has a value
      fail("expression member without a value");
    },
    [this](const Array& array) {
      // a literal states no sizes, so that the array is open
      return Schema( ArrayType{ std::make_shared<const Schema>( schemaOf(array.elements.front()) ), std::nullopt } );
    },
    [this](const Fields& fields) {
      ObjectType object;
      for ( auto& [name, member] : fields.members ) {
        object.fields.push_back( Field{ name, std::make_shared<const Schema>( schemaOf(member) ) } );
      }
      return Schema( std::move(object) );
    }
  }, literal.node);
}

void Scan::flatten(const Literal& literal, std::vector<Value>& values, size_t offset) const {
  std::visit(Overloaded{
    [&](const Scalar& scalar) {
      values[offset] = number(scalar.value);
    },
    [this](const Expression&) {
      fail("expression member without a value");
    },
    [&](const Array& array) {
      size_t span = layoutOf(array.elements.front()).size();
      for ( size_t i = 0; i < array.elements.size(); i++ ) {
        flatten(array.elements[i], values, offset + i * span);
      }
    },
    [&](const Fields& fields) {
      size_t memberOffset = 0;
      for ( auto& [name, member] : fields.members ) {
        flatten(member, values, offset + memberOffset);
        memberOffset += layoutOf(member).size();
      }
    }
  }, literal.node);
}

std::string Scan::scanMembershipOperator() const {
  for ( auto& operator_ : membershipOperators ) {
    if ( input.compare(position, operator_.size(), operator_) == 0 ) {
      return operator_;
    }
  }
  return {};
}

void Scan::recordLiteral(size_t begin, bool object) {
  literals++;
  literalBegin = begin;
  literalEnd = position;
  literalObject = object;
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

InputEncoder::InputEncoder(const std::string& input, bool initializers) {
  Scan scan(input, initializers);
  scan.run();
  encoded = scan.result();
  literalObject = scan.getObjectIndex();
  literalObjects = scan.getObjectIndices();
  encodedLiterals = scan.getEncodedLiterals();
}

InputEncoder::InputEncoder(std::string text, std::nullopt_t)
  : encoded(std::move(text))
{
}

InputEncoder InputEncoder::fragment(std::string text) {
  return InputEncoder(std::move(text), std::nullopt);
}

namespace {

/// Collects the expression members of a literal in the order in which they are written, each with its path.
void collect(const Literal& literal, ObjectInitializer::Path& path, std::vector<ObjectInitializer::Member>& members) {
  std::visit(Overloaded{
    [](const Scalar&) {},
    [&](const Expression& expression) {
      members.push_back( ObjectInitializer::Member{ path, expression.text } );
    },
    [&](const Array& array) {
      path.push_back("");
      for ( auto& element : array.elements ) {
        collect(element, path, members);
      }
      path.pop_back();
    },
    [&](const Fields& fields) {
      for ( auto& [name, member] : fields.members ) {
        path.push_back(name);
        collect(member, path, members);
        path.pop_back();
      }
    }
  }, literal.node);
}

/// Returns the literal with every expression member replaced by the scalar given for it, in the order in which
/// the members are collected.
Literal give(const Literal& literal, const std::vector<Scalar>& values, size_t& next) {
  return std::visit(Overloaded{
    [](const Scalar& scalar) {
      return Literal{ scalar };
    },
    [&](const Expression&) {
      return Literal{ values[next++] };
    },
    [&](const Array& array) {
      Array given;
      for ( auto& element : array.elements ) {
        given.elements.push_back( give(element, values, next) );
      }
      return Literal{ std::move(given) };
    },
    [&](const Fields& fields) {
      Fields given;
      for ( auto& [name, member] : fields.members ) {
        given.members.emplace_back( name, give(member, values, next) );
      }
      return Literal{ std::move(given) };
    }
  }, literal.node);
}

} // namespace

ObjectInitializer::ObjectInitializer(const std::string& text)
  : text(text)
{
  Scan scan(this->text, true);
  auto read = std::make_shared<const InputEncoder::Literal>( scan.scanInitializer() );
  Path path;
  collect(*read, path, expressionMembers);
  literal = std::move(read);
}

ObjectInitializer::ObjectInitializer(size_t constant)
  : constant(constant)
{
}

std::pair<std::shared_ptr<const Object>, Schema> ObjectInitializer::instantiate(const std::vector<std::pair<double, ValueType>>& values) const {
  if ( constant.has_value() ) {
    return { objectRegistry[constant.value()], objectRegistry.schema(constant.value()) };
  }
  assert( values.size() == expressionMembers.size() );

  std::vector<Scalar> scalars;
  for ( auto& [value, type] : values ) {
    scalars.push_back( Scalar{ value, type } );
  }
  size_t next = 0;
  Literal instance = give(*literal, scalars, next);

  // an object is created modifiable, so that a write may change it in place once it is no longer shared
  Scan scan(text);
  auto object = std::make_shared<Object>();
  object->layout = std::make_shared<const Object::Layout>( scan.layoutOf(instance) );
  object->values.resize( object->layout->size() );
  scan.flatten(instance, object->values, 0);
  return { std::move(object), scan.schemaOf(instance) };
}
