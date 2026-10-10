#include "Number.h"
#include "Value.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include "ObjectRegistry.h"
#include "InputEncoder.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <cassert>

namespace BPMNOS { 

Data::Data(const SharedData& data) {
  attributes.reserve(data.attributes.size());
  for (const auto& value : data.attributes) {
    attributes.push_back(value.get());
  }
  objects.reserve(data.objects.size());
  for (const auto& object : data.objects) {
    objects.push_back(object.get());
  }
}

SharedData::SharedData(const SharedData& other, Data& data)
  : SharedData(other)
{
  add(data);
}

SharedData::SharedData(Data& data) {
  add(data);
}

void SharedData::add(Data& data) {
  for ( auto& value : data.attributes ) {
    attributes.push_back(value);
  }
  for ( auto& object : data.objects ) {
    objects.push_back(object);
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
      // it is assumed that all collections are already encoded
      return number(BPMNOS::stoi( valueString ));
  }
  throw std::logic_error("to_number: unknown value type " + std::to_string(static_cast<int>(type)) );
}

number to_number(const ValueVariant& value, const ValueType& type) {
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
        return number(std::get<double>(value) != 0.0 ? 1 : 0);
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
        {
          // the text must state a collection, the number encoding it being meaningless otherwise
          InputEncoder encoder( std::get<std::string>(value) );
          if ( encoder.type() != ValueType::COLLECTION ) {
            throw std::runtime_error("to_number: '" + std::get<std::string>(value) + "' is no collection" );
          }
          return number( BPMNOS::stoi( encoder.text() ) );
        }
      }
      else [[unlikely]] {
        throw std::logic_error("to_number: illegal conversion" );
      }
  }
  throw std::logic_error("to_number: unknown value type " + std::to_string(static_cast<int>(type)) );
}

Value to_value(std::optional<double> result) {
  if ( !result.has_value() ) {
    return std::nullopt;
  }
  return number(result.value());
}

std::string to_string(number numericValue, const ValueType& type) {
  switch ( type ) {
    case ValueType::BOOLEAN:
      return numericValue ? Keyword::True : Keyword::False;
    case ValueType::INTEGER:
      return std::to_string((int)numericValue);
    case ValueType::DECIMAL:
      return BPMNOS::to_string((double)numericValue);
    case ValueType::STRING:
      return stringRegistry[(std::size_t)numericValue];
    case ValueType::COLLECTION:
      // a collection is a constant object, rendered as the literal stating it
      return BPMNOS::to_string( *objectRegistry[(std::size_t)numericValue] );
  }
  throw std::logic_error("to_string: unknown value type " + std::to_string(static_cast<int>(type)) );
}

std::string to_string(double value) {
  std::string result = std::to_string(value);
  if ( result.contains('.') ) {
    while ( result.back() == '0' ) {
      result.pop_back();
    }
    if ( result.back() == '.' ) {
      result.pop_back();
    }
  }
  return result;
}

BPMNOS::Status mergeStatus(const std::vector<BPMNOS::Status>& statuses) {
  assert( !statuses.empty() );
  size_t n = statuses.front().attributes.size();
  BPMNOS::Status result(n);
  result.attributes[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp] = statuses.front().attributes[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp];

  for ( size_t i = 0; i < n; i++ ) {
    for ( auto& status : statuses ) {
      if ( i == (int)BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        if ( result.attributes[i].value() < status.attributes[i].value() ) {
          result.attributes[i] = status.attributes[i];
        }
      }
      else if ( !result.attributes[i].has_value() ) {
        result.attributes[i] = status.attributes[i];
      }
      else if ( status.attributes[i].has_value() && status.attributes[i].value() != result.attributes[i].value() ) {
        result.attributes[i] = std::nullopt;
        break;
      }
    }
  }
  return result;
}

} // namespace BPMNOS::Model
