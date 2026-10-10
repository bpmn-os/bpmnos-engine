#include "Number.h"
#include "Value.h"
#include "Keywords.h"
#include "StringRegistry.h"
#include "ObjectRegistry.h"
#include "InputEncoder.h"
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
      // the string registry holds false at index 0 and true at index 1
      if ( valueString != Keyword::False && valueString != Keyword::True ) {
        throw std::runtime_error("to_number: '" + valueString + "' is no truth value");
      }
      return number(stringRegistry( valueString ));
    case ValueType::INTEGER:
      return number(BPMNOS::stoi( valueString ));
    case ValueType::DECIMAL:
      return number(BPMNOS::stod( valueString ));
    case ValueType::STRING:
      return number(stringRegistry( valueString ));
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


} // namespace BPMNOS::Model
