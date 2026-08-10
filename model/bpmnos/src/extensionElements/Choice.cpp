#include "Choice.h"
#include "model/utility/src/CollectionRegistry.h"
#include <cmath>
#include "model/utility/src/string_utility.h"
#include "model/utility/src/InputEncoder.h"

using namespace BPMNOS::Model;

Choice::Choice(XML::bpmnos::tDecision* decision, const AttributeRegistry& attributeRegistry)
  : element(decision)
  , attributeRegistry(attributeRegistry)
  , attribute(nullptr)
{
  // The membership operator is written out before the condition is scanned, so that the scan meets the
  // name `in` and reads the brackets of an enumeration as an index, which it copies. Were the operator
  // written out afterwards, the brackets would follow no name and the enumeration would be read as a
  // collection literal, which it is not: its alternatives may be expressions.
  auto condition = decision->condition.value.value;
  BPMNOS::replace_all( condition, "∈", " in ");
  auto input = InputEncoder(condition).text();

  if ( input.contains(" in ") ) {
    parseEnumeration(input);
  }
  else if ( !input.contains("<") ) {
    throw std::runtime_error("Choice: no enumeration or bounds given in '" + input + "'");
  }
  else {  
    BPMNOS::replace_all( input, "|", " divides ");
    auto [bounds,discretizer] = [&input]() -> std::pair<std::string, std::string> {
      if ( !input.contains(" divides ") ) {
        // no discretizer provided
        return { input, "" };
      }
      
      // discretizer is assumed to be provided following the bounds, separated by comma
      auto pos = input.rfind(',');
      if ( pos == std::string::npos ) {
        throw std::runtime_error("Choice: illegal condition '" + input + "'");
      }
      return { 
        BPMNOS::trim_copy(input.substr(0, pos)), 
        BPMNOS::trim_copy(input.substr(pos + 1))
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
  auto parts = BPMNOS::split(input," in ");
  if ( parts.size() != 2 ) {
    throw std::runtime_error("Choice: illegal enumeration '" + input + "'");
  }

  std::string attributeName = BPMNOS::trim_copy(parts.front());
  if ( attributeName == "" ) {
    throw std::runtime_error("Choice: unable to determine attribute name");
  }
  attribute = attributeRegistry[ attributeName ];

  auto rhs = BPMNOS::trim_copy(parts.back());

  if ( (rhs.front() == '[' && rhs.back() == ']') || (rhs.front() == '{' && rhs.back() == '}') ) {
    // the condition has been scanned, so a literal among the alternatives is a number already and the
    // commas that remain are the ones separating the alternatives
    auto alternatives = BPMNOS::split( rhs.substr(1, rhs.size()-2), ',' );
    for ( auto& alternative : alternatives ) {
      enumeration.emplace_back( std::make_unique<Expression>(InputEncoder::fragment(BPMNOS::trim_copy(alternative)), attributeRegistry) );
      for ( auto dependency : enumeration.back()->inputs ) {
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
  auto conditions = BPMNOS::split(input,'<');
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
    lowerBound.emplace(
      std::make_unique<Expression>(InputEncoder::fragment(BPMNOS::trim_copy(conditions[0])), attributeRegistry),
      strictLB
    );
    for ( auto dependency : lowerBound.value().first->inputs ) {
      dependencies.insert(dependency);
    }
    
    // determine attribute
    std::string attributeName = BPMNOS::trim_copy(conditions[1]);
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

    upperBound.emplace(
      std::make_unique<Expression>(InputEncoder::fragment(BPMNOS::trim_copy(conditions[2])), attributeRegistry),
      strictUB
    );
    for ( auto dependency : upperBound.value().first->inputs ) {
      dependencies.insert(dependency);
    }

  }
  else {
    // unbounded
    throw std::runtime_error("Choice: condition '" + input + "' is unbounded");
  }
}

void Choice::parseDiscretizer(const std::string& input) {
  auto parts = BPMNOS::split(input," divides ");
  std::string attributeName = BPMNOS::trim_copy(parts[1]);
  if ( attribute->name != attributeName ) {
    throw std::runtime_error("Choice: inconsistent attribute name '" + attributeName + "' in '" + input + "'");
  }
  multipleOf = std::make_unique<Expression>(InputEncoder::fragment(BPMNOS::trim_copy(parts[0])), attributeRegistry);
  for ( auto dependency : multipleOf->inputs ) {
    dependencies.insert(dependency);
  }
}


template <typename DataType>
std::pair<BPMNOS::number,BPMNOS::number> Choice::getBounds(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  assert( attribute->type != STRING );
  assert( lowerBound.has_value() );  
  assert( upperBound.has_value() );
  auto& [LB,strictLB] = lowerBound.value();
  auto lb = LB->execute(status,data,globals);
  BPMNOS::number min = lb.has_value() ? BPMNOS::number(lb.value()) : std::numeric_limits<BPMNOS::number>::lowest();
  if ( strictLB ) {
    min += BPMNOS_NUMBER_PRECISION;
  }

  auto& [UB,strictUB] = upperBound.value();
  auto ub = UB->execute(status,data,globals);
  BPMNOS::number max = ub.has_value() ? BPMNOS::number(ub.value()) : std::numeric_limits<BPMNOS::number>::max();
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
  assert( !enumeration.empty() || multipleOf );  
  std::vector<BPMNOS::number> allowedValues;
  if ( !enumeration.empty() ) {
    for ( auto& alternative : enumeration ) {
      auto allowedValue = alternative->execute(status,data,globals);
      if ( allowedValue.has_value() ) {
        // an alternative is a value the attribute may take, so it is held as such
        allowedValues.push_back( BPMNOS::number(allowedValue.value()) );
      }
    }
  }
  else {
    auto [LB, UB] = getBounds(status, data, globals);
    auto discretizer = multipleOf->execute(status, data, globals);
    if ( !discretizer.has_value() ) {
      if ( attribute->type == BPMNOS::ValueType::BOOLEAN || attribute->type == BPMNOS::ValueType::INTEGER ) {
        discretizer = 1;
      }
      else {
        throw std::runtime_error("Choice: cannot determine discretizer for '" + multipleOf->expression + "'");
      }
    }
    // The discretizer is kept at the precision it was evaluated at, so that the multiples are multiples of
    // the step the model states rather than of a step already rounded: a third rounded first would put the
    // whole grid a millionth below the thirds, and further below with every multiple.
    double DELTA = std::abs(discretizer.value());
    if ( DELTA <= 0.0 ) {
      throw std::runtime_error("Choice: non-positive discretizer for '" + multipleOf->expression + "'");
    }

    // The multiples are counted from zero, and each is a value the attribute may take and is therefore
    // rounded as it is stored. Whether a multiple lies within the bounds is decided on that value and not on
    // the double it is computed from, the two falling on opposite sides of a bound the value lies exactly
    // on. The walk starts one multiple below what the bound suggests for the same reason: dividing in
    // floating point puts a bound that is exactly a multiple just above one, and rounding the quotient up
    // would step over the bound itself.
    for ( double multiple = std::floor((double)LB / DELTA); ; multiple += 1.0 ) {
      BPMNOS::number value = multiple * DELTA;
      if ( value > UB ) {
        break;
      }
      if ( value >= LB ) {
        allowedValues.push_back(value);
      }
    }
  }
  
  return allowedValues;
}

template std::vector<BPMNOS::number> Choice::getEnumeration<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template std::vector<BPMNOS::number> Choice::getEnumeration<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

