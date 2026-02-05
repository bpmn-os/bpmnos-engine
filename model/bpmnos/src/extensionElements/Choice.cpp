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
  auto input = encodeQuotedStrings(decision->condition.value.value);
  strutil::replace_all( input, "∈", " in ");

  if ( strutil::contains(input," in ") ) {
    parseEnumeration(input);
  }
  else if ( !strutil::contains(input,"<") ) {
    throw std::runtime_error("Choice: no enumeration or bounds given in '" + input + "'");
  }
  else {  
    strutil::replace_all( input, "|", " divides ");
    auto [bounds,discretizer] = [&input]() -> std::pair<std::string, std::string> {
      if ( !strutil::contains(input," divides ") ) {
        // no discretizer provided
        return { input, "" };
      }
      
      // discretizer is assumed to be provided following the bounds, separated by comma
      auto pos = input.rfind(',');
      if ( pos == std::string::npos ) {
        throw std::runtime_error("Choice: illegal condition '" + input + "'");
      }
      return { 
        strutil::trim_copy(input.substr(0, pos)), 
        strutil::trim_copy(input.substr(pos + 1))
      };
    }();

    parseBounds(bounds);

    if ( !discretizer.empty() ) {
      parseDiscretizer(discretizer);
    }
  }
    
  if ( attribute->type == STRING && enumeration.empty() ) {
    throw std::runtime_error("Choice: no enumeration provided for string");
  }
  if ( attribute->type == COLLECTION ) {
    throw std::runtime_error("Choice: attribute is a collection");
  }

  attribute->isImmutable = false;
}

void Choice::parseEnumeration(const std::string& input) {
  auto parts = strutil::split(input," in ");
  if ( parts.size() != 2 ) {
    throw std::runtime_error("Choice: illegal enumeration '" + input + "'");
  }

  std::string attributeName = strutil::trim_copy(parts.front());
  if ( attributeName == "" ) {
    throw std::runtime_error("Choice: unable to determine attribute name");
  }
  attribute = attributeRegistry[ attributeName ];

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

void Choice::parseBounds(const std::string& input) {
  // check bounds
  auto conditions = strutil::split(input,'<');
  if ( conditions.size() == 3 ) {
    bool strictLB = false;
    // condition has two inequalities
    if ( conditions[1][0] == '=' ) {
      // inequality, remove '=' and trim
      conditions[1].erase(0, 1);
    }
    else {
      // strict inequality
      strictLB = true;
    }
    lowerBound.emplace(std::piecewise_construct,
      std::forward_as_tuple(strutil::trim_copy(conditions[0]), attributeRegistry),
      std::forward_as_tuple(strictLB)
    );
    for ( auto dependency : lowerBound.value().first.inputs ) {
      dependencies.insert(dependency);
    }
    
    // determine attribute
    std::string attributeName = strutil::trim_copy(conditions[1]);
    if ( attributeName == "" ) {
      throw std::runtime_error("Choice: unable to determine attribute name");
    }
    attribute = attributeRegistry[ attributeName ];
    
    bool strictUB = false;
    if ( conditions[2][0] == '=' ) {
      // inequality, remove '=' and trim
      conditions[2].erase(0, 1);
    }
    else {
      // strict inequality
      strictUB = true;
    }

    upperBound.emplace(std::piecewise_construct,
      std::forward_as_tuple(strutil::trim_copy(conditions[2]), attributeRegistry),
      std::forward_as_tuple(strictUB)
    );
    for ( auto dependency : upperBound.value().first.inputs ) {
      dependencies.insert(dependency);
    }

  }
  else {
    // unbounded
    throw std::runtime_error("Choice: condition '" + input + "' is unbounded");
  }
}

void Choice::parseDiscretizer(const std::string& input) {
  auto parts = strutil::split(input," divides ");
  std::string attributeName = strutil::trim_copy(parts[1]);
  if ( attribute->name != attributeName ) {
    throw std::runtime_error("Choice: inconsistent attribute name '" + attributeName + "' in '" + input + "'");
  }
  multipleOf.emplace( strutil::trim_copy(parts[0]), attributeRegistry);
  for ( auto dependency : multipleOf.value().inputs ) {
    dependencies.insert(dependency);
  }
}


template <typename DataType>
std::pair<BPMNOS::number,BPMNOS::number> Choice::getBounds(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  assert( attribute->type != STRING );
  assert( lowerBound.has_value() );  
  assert( upperBound.has_value() );
  auto& [LB,strictLB] = lowerBound.value();   
  BPMNOS::number min =  LB.execute(status,data,globals).value_or(std::numeric_limits<BPMNOS::number>::lowest());
  if ( strictLB ) {
    min += BPMNOS_NUMBER_PRECISION;
  }

  auto& [UB,strictUB] = upperBound.value();   
  BPMNOS::number max = UB.execute(status,data,globals).value_or(std::numeric_limits<BPMNOS::number>::max());
  if ( strictUB ) {
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

