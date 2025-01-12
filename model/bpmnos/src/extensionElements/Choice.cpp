#include "Choice.h"
#include "model/utility/src/CollectionRegistry.h"
#include <cmath>
#include <strutil.h>
#include "model/utility/src/encode_quoted_strings.h"
#include "model/utility/src/encode_collection.h"

using namespace BPMNOS::Model;

Choice::Choice(XML::bpmnos::tDecision* decision, const AttributeRegistry& attributeRegistry)
  : element(decision)
  , attributeRegistry(attributeRegistry)
  , attribute(nullptr)
{
  std::string attributeName;
  auto input = encodeQuotedStrings(decision->condition.value.value);
  strutil::replace_all( input, "∈", " in ");

  // check whether condition provides bounds or enumeration
  auto conditions = strutil::split(input,'<');
  if ( conditions.size() == 3 ) {
    // condition has two inequalities
    if ( conditions[1][0] == '=' ) {
      // inequality, remove '=' and trim
      conditions[1].erase(0, 1);
      strictness.first = false;
    }
    else {
      // strict inequality
      strictness.first = true;
    }
    lowerBound.emplace( strutil::trim_copy(conditions[0]), attributeRegistry);
    for ( auto dependency : lowerBound.value().inputs ) {
      dependencies.insert(dependency);
    }
    
    // determine attribute
    attributeName = strutil::trim_copy(conditions[1]);
    
    if ( conditions[2][0] == '=' ) {
      // inequality, remove '=' and trim
      conditions[2].erase(0, 1);
      strictness.second = false;
    }
    else {
      // strict inequality
      strictness.second = true;
    }

    upperBound.emplace( strutil::trim_copy(conditions[2]), attributeRegistry);
    for ( auto dependency : upperBound.value().inputs ) {
      dependencies.insert(dependency);
    }

  }
  else if ( auto parts = strutil::split(input," in "); parts.size() == 2 ) {

    attributeName = strutil::trim_copy(parts.front());
    auto rhs = strutil::trim_copy(parts.back());

    if ( (rhs.front() == '[' && rhs.back() == ']') || (rhs.front() == '{' && rhs.back() == '}') ) {
      auto alternatives = strutil::split( encodeCollection( rhs.substr(1, rhs.size()-2) ), ',' );
      for ( auto& alternative : alternatives ) {
        enumeration.emplace_back( strutil::trim_copy(alternative), attributeRegistry);
        for ( auto dependency : enumeration.back().inputs ) {
          dependencies.insert(dependency);
        }
      }
      if ( enumeration.empty() ) {
        throw std::runtime_error("Choice: empty enumeration");
      }
    }
    else {
      throw std::runtime_error("Choice: invalid enumeration '" + rhs + "'");
    }
  }
  else {
    attributeName = strutil::trim_copy(input);
    lowerBound.emplace("false", attributeRegistry);
    upperBound.emplace("true", attributeRegistry);
  }
  
  if ( attributeName == "" ) {
    throw std::runtime_error("Choice: unable to determine attribute name");
  }

  attribute = attributeRegistry[ attributeName ];
  
  if ( attribute->type == STRING && enumeration.empty() ) {
    throw std::runtime_error("Choice: no enumeration provided for string");
  }
  if ( attribute->type == COLLECTION ) {
    throw std::runtime_error("Choice: attribute is a collection");
  }

  attribute->isImmutable = false;
}

template <typename DataType>
std::pair<BPMNOS::number,BPMNOS::number> Choice::getBounds(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  assert( attribute->type != STRING );
  assert( lowerBound.has_value() );  
  assert( upperBound.has_value() );  
  BPMNOS::number min = lowerBound.value().execute(status,data,globals).value_or(std::numeric_limits<BPMNOS::number>::min());
  if ( strictness.first ) {
    min += BPMNOS_NUMBER_PRECISION;
  }
  BPMNOS::number max = upperBound.value().execute(status,data,globals).value_or(std::numeric_limits<BPMNOS::number>::max());
  if ( strictness.second ) {
    max -= BPMNOS_NUMBER_PRECISION;
  }
  if ( attribute->type != DECIMAL ) {
    min = std::ceil((double)min);
    max = std::floor((double)max);
  }

  return {min,max};
}

template std::pair<BPMNOS::number,BPMNOS::number>  Choice::getBounds<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template std::pair<BPMNOS::number,BPMNOS::number>  Choice::getBounds<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


template <typename DataType>
std::vector<BPMNOS::number> Choice::getEnumeration(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  assert( enumeration.size() );  
  std::vector<BPMNOS::number> allowedValues;
  for ( auto& alternative : enumeration ) {
    auto allowedValue = alternative.execute(status,data,globals);
    if ( allowedValue.has_value() ) {
      allowedValues.push_back( allowedValue.value() );
    }
  }

  return allowedValues;
}

template std::vector<BPMNOS::number> Choice::getEnumeration<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template std::vector<BPMNOS::number> Choice::getEnumeration<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

