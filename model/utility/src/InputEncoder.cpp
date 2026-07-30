#include "InputEncoder.h"

#include <cctype>
#include <stdexcept>
#include <utility>
#include <vector>

#include "CollectionRegistry.h"
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

private:
  /// Reads a quoted span and returns its index in the string registry.
  size_t scanString();
  /// Reads a collection literal and returns its index in the collection registry.
  size_t scanCollection();
  /// Reads one member of a collection literal and returns its value and the type it is recorded as.
  std::pair<double, ValueType> scanMember();
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
    else if ( character == '[' && literalAllowed ) {
      size_t begin = position;
      output += std::to_string( scanCollection() );
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

size_t Scan::scanCollection() {
  ++position; // skip the opening bracket

  std::vector<double> members;
  std::optional<ValueType> memberType;

  skipSpace();
  if ( position < input.size() && input[position] == ']' ) {
    fail("collection without members");
  }

  while ( true ) {
    skipSpace();
    auto [value, type] = scanMember();

    if ( !memberType.has_value() ) {
      memberType = type;
    }
    else if ( memberType.value() != type ) {
      fail("members of different type");
    }
    members.push_back(value);

    skipSpace();
    if ( position >= input.size() ) {
      fail("unterminated collection");
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

  return collectionRegistry(members, memberType.value());
}

std::pair<double, ValueType> Scan::scanMember() {
  if ( position >= input.size() ) {
    fail("unterminated collection");
  }

  if ( input[position] == '"' ) {
    return { static_cast<double>( scanString() ), STRING };
  }

  if ( input[position] == '[' ) {
    return { static_cast<double>( scanCollection() ), COLLECTION };
  }

  size_t begin = position;
  while ( position < input.size() && input[position] != ',' && input[position] != ']' ) {
    ++position;
  }

  std::string member = BPMNOS::trim_copy( input.substr(begin, position - begin) );

  if ( member.empty() ) {
    fail("member without value");
  }
  if ( member == Keyword::False ) {
    return { 0.0, BOOLEAN };
  }
  if ( member == Keyword::True ) {
    return { 1.0, BOOLEAN };
  }

  // a number is recorded as a decimal, so that whole and fractional members agree in type
  size_t consumed = 0;
  double value = 0.0;
  try {
    value = std::stod(member, &consumed);
  }
  catch ( const std::exception& ) {
    fail("illegal member '" + member + "'");
  }
  if ( consumed != member.size() ) {
    fail("illegal member '" + member + "'");
  }

  return { value, DECIMAL };
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
}

InputEncoder::InputEncoder(std::string text, std::nullopt_t)
  : encoded(std::move(text))
  , literalType(std::nullopt)
{
}

InputEncoder InputEncoder::fragment(std::string text) {
  return InputEncoder(std::move(text), std::nullopt);
}
