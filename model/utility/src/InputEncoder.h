#ifndef BPMNOS_InputEncoder_H
#define BPMNOS_InputEncoder_H

#include <optional>
#include <string>
#include <vector>

#include "Value.h"

namespace BPMNOS {

/**
 * @brief Utility class replacing every literal in a text by the number encoding it.
 *
 * The engine represents a string by its index in the string registry and a collection by its index in
 * the collection registry, so that every value is a number. Text stating such a value must therefore be
 * scanned before it is read as a value or parsed as an expression, and this class is the one place in
 * which that is done, so that what a lexeme is is decided once.
 *
 * The text is read once, from left to right, and each thing met is dealt with where it is met.
 *
 * A quoted span is read to its closing quote and emitted as its index in the string registry. A quote
 * within it does not occur, quotes not being escaped, and a comma or a bracket within it is text rather
 * than structure.
 *
 * A bracket either opens a collection literal or belongs to the expression it stands in, and which of
 * the two is decided by what has been emitted last. After a name, a closing bracket or a closing
 * parenthesis it indexes a collection, and after a membership operator it states a collection the
 * expression parser reads itself, so in both cases the bracketed text is copied and its contents are
 * scanned where they stand. Everywhere else the bracket opens a literal. The members of a literal are read one by one, each by the same rules, so that
 * a member is a quoted string, a truth value, a number, or a literal of its own, to any depth. The
 * members must agree in the type they are recorded as, a member that is a literal being recorded as a
 * collection whatever it holds, and the literal is registered with the type they agree on and emitted as
 * its index in the collection registry. A numeric member is recorded as a decimal, so that a literal may
 * hold whole and fractional numbers alike and both are written out as they were read.
 *
 * Everything else, which is to say names, numbers, operators, whitespace, and the delimiters of a line,
 * is copied unchanged, a membership operator included. Of the grammar the expression parser applies, the
 * scan knows what decides a bracket and nothing besides, so that the text it emits is read as the text
 * it was given.
 *
 * A text that cannot be read is refused, naming it: a quote that is not closed, a literal that is not
 * closed, a literal without members, a member without a value, a member that is neither a truth value
 * nor a number nor a literal, and members that do not agree in type.
 */
class InputEncoder {
public:
  /// Constructor scanning the given text.
  InputEncoder(const std::string& input);

  /// Returns the text with every literal replaced by the number encoding it.
  const std::string& text() const { return encoded; }

  /// Returns the type of the literal if the text states one literal and nothing besides whitespace, and
  /// no value otherwise. This is what a caller needs where the text states a value rather than an
  /// expression, the type of an expression following from its shape rather than from its text.
  const std::optional<ValueType>& type() const { return literalType; }

  /// Returns a part of a text that has been scanned already, such as one alternative of an enumeration.
  /// No type is stated, a part of a text being no literal of its own.
  static InputEncoder fragment(std::string text);

private:
  /// Constructor adopting text that has been scanned already.
  InputEncoder(std::string text, std::nullopt_t);

  std::string encoded;
  std::optional<ValueType> literalType;
};

} // namespace BPMNOS

#endif // BPMNOS_InputEncoder_H
