#include "Number.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include <cassert>

namespace BPMNOS { 

number to_number(const std::string& valueString, const ValueType& type) {
  switch ( type ) {
    case ValueType::STRING:
      return number(stringRegistry( valueString ));
    case ValueType::BOOLEAN:
      return number(stringRegistry( valueString ));
    case ValueType::INTEGER:
      return number(std::stoi( valueString ));
    case ValueType::DECIMAL:
      return number(std::stod( valueString ));
  }
  throw std::logic_error("to_number: unknown value type " + type );
}

number to_number(const Value& value, const ValueType& type) {
  switch ( type ) {
    case ValueType::STRING:
      if (std::holds_alternative<std::string>(value)) [[likely]] {
        return number(stringRegistry(std::get<std::string>(value)));
      }
      else if (std::holds_alternative<bool>(value)) {
        return number(std::get<bool>(value) ? 1 : 0);
      }
      else if (std::holds_alternative<int>(value)) {
        return number(stringRegistry(std::to_string(std::get<int>(value))));
      }
      else if (std::holds_alternative<double>(value)) {
        return number(stringRegistry( std::to_string(std::get<double>(value))));
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: value holds no alternative" );
      }
    case ValueType::BOOLEAN:
      if (std::holds_alternative<std::string>(value)) {
        return number(std::get<std::string>(value) == Keyword::True ? 1 : 0);
      }
      else if (std::holds_alternative<bool>(value)) [[likely]] {
        return number(std::get<bool>(value) ? 1 : 0);
      }
      else if (std::holds_alternative<int>(value)) {
        return number(std::get<int>(value) ? 1 : 0);
      }
      else if (std::holds_alternative<double>(value)) {
        return number(std::get<double>(value) ? 1 : 0);
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: value holds no alternative" );
      }
    case ValueType::INTEGER:
      if (std::holds_alternative<std::string>(value)) {
        return number(std::stoi(std::get<std::string>(value)));
      }
      else if (std::holds_alternative<bool>(value)) {
        return number(std::get<bool>(value) ? 1 : 0);
      }
      else if (std::holds_alternative<int>(value)) [[likely]] {
        return number(std::get<int>(value));
      }
      else if (std::holds_alternative<double>(value)) {
        return number((int)std::get<double>(value));
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: value holds no alternative" );
      }
    case ValueType::DECIMAL:
      if (std::holds_alternative<std::string>(value)) {
        return number(std::stod(std::get<std::string>(value)));
      }
      else if (std::holds_alternative<bool>(value)) {
        return number(std::get<bool>(value) ? 1 : 0);
      }
      else if (std::holds_alternative<int>(value)) {
        return number(std::get<int>(value));
      }
      else if (std::holds_alternative<double>(value)) [[likely]] {
        return number(std::get<double>(value));
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: value holds no alternative" );
      }
  }
  throw std::logic_error("to_number: unknown value type " + type );
}

std::string to_string(number numberValue, const ValueType& type) {
  switch ( type ) {
    case ValueType::STRING:
      return stringRegistry[(std::size_t)numberValue];
    case ValueType::BOOLEAN:
      return numberValue ? Keyword::True : Keyword::False;
    case ValueType::INTEGER:
      return std::to_string((int)numberValue);
    case ValueType::DECIMAL:
      return std::to_string((double)numberValue);
  }
  throw std::logic_error("to_string: unknown value type " + type );
}

} // namespace BPMNOS::Model
