#include "Number.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include "CollectionRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <cassert>

namespace BPMNOS { 

Values::Values(const SharedValues& values) {
  for (const auto& value : values) {
    if (value.get().has_value()) {
      push_back(value.get().value());
    }
    else {
      push_back(std::nullopt);
    }
  }
}

SharedValues::SharedValues(const SharedValues& other,Values& values)
  : SharedValues(other)
{
  add(values);
}

SharedValues::SharedValues(Values& values) {
  add(values);
}

void SharedValues::add(Values& values) {
  for ( auto& value : values ) {
    push_back(value);
  }
}

double stod(const std::string& str) {
  try {
    double result = std::stod(str);
    return result;
  }
  catch( ... ) {
    throw std::runtime_error("Cannot convert '" + str + "' to double" );
  }
}

int stoi(const std::string& str) {
  try {
    int result = std::stoi(str);
    return result;
  }
  catch( ... ) {
    throw std::runtime_error("Cannot convert '" + str + "' to int" );
  }
}

number to_number(const std::string& valueString, const ValueType& type) {
  switch ( type ) {
    case ValueType::BOOLEAN:
      return number(stringRegistry( valueString ));
    case ValueType::INTEGER:
      return number(BPMNOS::stoi( valueString ));
    case ValueType::DECIMAL:
      return number(BPMNOS::stod( valueString ));
    case ValueType::STRING:
      return number(stringRegistry( valueString ));
    case ValueType::COLLECTION:
      return number(collectionRegistry( valueString ));
  }
  throw std::logic_error("to_number: unknown value type " + type );
}

number to_number(const Value& value, const ValueType& type) {
  switch ( type ) {
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
        return number(BPMNOS::stoi(std::get<std::string>(value)));
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
        return number(BPMNOS::stod(std::get<std::string>(value)));
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
    case ValueType::COLLECTION:
      if (std::holds_alternative<std::string>(value)) [[likely]] {
        return number(collectionRegistry(std::get<std::string>(value)));
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: illegal parameters" );
      }
  }
  throw std::logic_error("to_number: unknown value type " + type );
}

std::string to_string(number numericValue, const ValueType& type) {
  switch ( type ) {
    case ValueType::BOOLEAN:
      return numericValue ? Keyword::True : Keyword::False;
    case ValueType::INTEGER:
      return std::to_string((int)numericValue);
    case ValueType::DECIMAL:
      return std::to_string((double)numericValue);
    case ValueType::STRING:
      return stringRegistry[(std::size_t)numericValue];
    case ValueType::COLLECTION:
      return collectionRegistry[(std::size_t)numericValue].collection;
  }
  throw std::logic_error("to_string: unknown value type " + type );
}

BPMNOS::Values mergeValues(const std::vector<BPMNOS::Values>& valueSets) {
  assert( !valueSets.empty() );
  size_t n = valueSets.front().size();
  BPMNOS::Values result;
  result.resize(n);
  result[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp] = valueSets.front()[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp];

  for ( size_t i = 0; i < n; i++ ) {
    for ( auto& values : valueSets ) {
      if ( i == (int)BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        if ( result[i].value() < values[i].value() ) {
          result[i] = values[i];
        }
      }
      else if ( !result[i].has_value() ) {
        result[i] = values[i];
      }
      else if ( values[i].has_value() && values[i].value() != result[i].value() ) {
        result[i] = std::nullopt;
        break;
      }
    }
  }
  return result;
}

} // namespace BPMNOS::Model
