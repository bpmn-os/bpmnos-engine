#ifndef BPMNOS_InputEncoder_H
#define BPMNOS_InputEncoder_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Value.h"
#include "Object.h"
#include "Schema.h"

namespace BPMNOS {

/**
 * @brief Utility class replacing every literal in a text by the number encoding it.
 *
 * The engine represents a string by its index in the string registry and a literal array or value with
 * fields by the index of the constant object it states in the object registry, so that every value is a
 * number. Text stating such a value must therefore be
 * scanned before it is read as a value or parsed as an expression, and this class is the one place in
 * which that is done, so that what a lexeme is is decided once.
 *
 * The text is read once, from left to right, and each thing met is dealt with where it is met.
 *
 * A quoted span is read to its closing quote and emitted as its index in the string registry. A quote
 * within it does not occur, quotes not being escaped, and a comma or a bracket within it is text rather
 * than structure.
 *
 * A bracket either opens an array literal or belongs to the expression it stands in, and which of the two
 * is decided by what has been emitted last. After a name, a closing bracket or a closing parenthesis it
 * indexes, and after a membership operator it states a set the expression parser reads itself, so in both
 * cases the bracketed text is copied and its contents are scanned where they stand. Everywhere else the
 * bracket opens a literal. A brace opens a literal with fields, `{ name := value, ... }`, where a literal may
 * stand and a name followed by `:=` follows it; any other brace is a set or the body of an aggregation and is
 * copied. The members of an array and the values of the fields are read one by one, each by the same rules,
 * so that each is a quoted string, a truth value, a number, or a literal of its own, to any depth. A numeric
 * value is recorded as a decimal, so that a literal may hold whole and fractional numbers alike and both are
 * written out as they were read. The members of an array are uniform: they agree in type and, being arrays
 * or values with fields themselves, in the sizes of their arrays and in their fields. The literal is
 * registered in the object registry as a constant object, stored flat, together with its constant schema, which
 * states the value types of the literal and leaves every array open, and emitted as its index there.
 *
 * Everything else, which is to say names, numbers, operators, whitespace, and the delimiters of a line,
 * is copied unchanged, a membership operator included. Of the grammar the expression parser applies, the
 * scan knows what decides a bracket and nothing besides, so that the text it emits is read as the text
 * it was given.
 *
 * A text that cannot be read is refused, naming it: a quote that is not closed, a literal that is not
 * closed, an array without members, a member without a value, a member that is neither a truth value nor a
 * number nor a literal, members that are not uniform, and a field without a name, without `:=` or stated
 * twice.
 *
 * A text may be scanned admitting object initializers, as the instance data are, where an object is given a
 * literal whose members may be expressions. A literal with an expression member is then neither registered nor
 * refused but copied unchanged, so that it is read by @ref ObjectInitializer where it initialises an object,
 * and its encoded literal tells a reader splitting the text that no delimiter within it separates anything.
 */
class InputEncoder {
public:
  /// Constructor scanning the given text, admitting object initializers if requested.
  InputEncoder(const std::string& input, bool initializers = false);

  /// Returns the text with every literal replaced by the number encoding it.
  const std::string& getEncodedInput() const { return encoded; }

  /// Returns the index in the object registry of the constant object if the text states one literal array or
  /// value with fields and nothing besides whitespace, and no value otherwise. This is what a caller needs where
  /// the text states an object rather than an expression.
  const std::optional<size_t>& getObjectIndex() const { return literalObject; }

  /// Where a literal stands in the text with every literal replaced: the positions of the first character of
  /// the number encoding it, or of an object initializer copied unchanged, and of the character following it, and
  /// what it is.
  struct EncodedLiteral {
    enum class Type { STRING, OBJECT, INITIALIZER };
    size_t begin;
    size_t end;
    Type type;
  };

  /// Returns where each literal stands in the text with every literal replaced, in their order, so that a
  /// reader splitting that text can tell a cell stating a literal from a cell stating a number.
  const std::vector<EncodedLiteral>& getEncodedLiterals() const { return encodedLiterals; }

  /// Returns the indices in the object registry of the constant objects the literals of the text state, in
  /// their order, so that a caller can tell a number standing for a constant object from a number. A part of
  /// a text scanned already states none.
  const std::vector<size_t>& getObjectIndices() const { return literalObjects; }

  /// Returns a part of a text that has been scanned already, such as one alternative of an enumeration.
  /// No type is stated, a part of a text being no literal of its own.
  static InputEncoder fragment(std::string text);

  /// The syntax tree of a literal as it is read, before it is registered as a constant object or instantiated: a
  /// scalar, an expression member of an object initializer, an array, or a value with fields. The types are
  /// defined where literals are read, so that no other code builds or inspects a tree.
  struct Literal;
  struct Scalar;
  struct Expression;
  struct Array;
  struct Fields;

private:
  /// Constructor adopting text that has been scanned already.
  InputEncoder(std::string text, std::nullopt_t);

  std::string encoded;
  std::optional<size_t> literalObject;
  std::vector<size_t> literalObjects;
  std::vector<EncodedLiteral> encodedLiterals;
};

/**
 * @brief An object literal whose members may be expressions, stating the value an object is initialised with.
 *
 * The literal is read by the rules of the literals of @ref InputEncoder, except that a member that is neither a
 * quoted string, a truth value, a number nor a literal is an expression. It reaches to the next comma, closing
 * bracket or closing brace standing outside every parenthesis, bracket, brace and quote, and its text is scanned
 * as the text of any expression. An object is instantiated by giving every expression member a value, whose type
 * is that of the attribute the object is given to at the position of the member, so that the members of an array
 * are uniform once their values are known. An initializer without expression members states a constant object.
 */
class ObjectInitializer {
public:
  /// Constructor reading the given text, which states one literal and nothing besides whitespace.
  ObjectInitializer(const std::string& text);
  /// Constructor for the constant with the given index in the object registry, which has no expression members.
  ObjectInitializer(size_t constant);

  /// A step of the path from the object to a member: the name of a field, or an empty name for an element of an
  /// array.
  using Path = std::vector<std::string>;

  /// An expression member: where it stands in the literal, and the text of its expression.
  struct Member {
    Path path;
    std::string expression;
  };

  /// Returns the expression members in the order in which they are written.
  const std::vector<Member>& members() const { return expressionMembers; }

  /// Returns the object the literal states and its schema, each expression member having the value given at its
  /// position in @ref members, with its type; the value of a string is its index in the string registry. The schema
  /// states the value types of the object and leaves every array open, as the constant schema of a literal does.
  std::pair<std::shared_ptr<const Object>, Schema> instantiate(const std::vector<std::pair<double, ValueType>>& values) const;

private:
  std::string text;
  std::shared_ptr<const InputEncoder::Literal> literal;
  std::optional<size_t> constant; ///< The index in the object registry of a constant, which has no expression members
  std::vector<Member> expressionMembers;
};

} // namespace BPMNOS

#endif // BPMNOS_InputEncoder_H
