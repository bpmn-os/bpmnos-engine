#include "LinearExpression.h"
#include "model/utility/src/Keywords.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

LinearExpression::LinearExpression(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap)
  : Expression(parameter, attributeMap)
{
  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("LinearExpression: expression must be given by value");
  }

  if ( strutil::contains(expression, "==") ) {
    type = Type::EQUAL;
    parseInequality("==");
  }
  else if ( strutil::contains(expression, "!=") ) {
    type = Type::NOTEQUAL;
    parseInequality("!=");
  }
  else if ( strutil::contains(expression, ">=") ) {
    type = Type::GREATEROREQUAL;
    parseInequality(">=");
  }
  else if ( strutil::contains(expression, ">") ) {
    type = Type::GREATERTHAN;
    parseInequality(">");
  }
  else if ( strutil::contains(expression, "<=") ) {
    type = Type::LESSOREQUAL;
    parseInequality("<=");
  }
  else if ( strutil::contains(expression, "<") ) {
    type = Type::LESSTHAN;
    parseInequality("<");
  }
  else {
    type = Type::DEFAULT;
    parse( expression );
  }
}

void LinearExpression::parse(std::string expressionString, NumericType SIGN) {
  expressionString.erase(remove(expressionString.begin(), expressionString.end(), ' '), expressionString.end()); // remove all spaces
  expressionString = std::regex_replace(expressionString, std::regex("-"), "+-"); // replace "-" by "+-"
  std::regex re1("[+]");
  std::sregex_token_iterator first{expressionString.begin(), expressionString.end(), re1, -1}, last; // split by "+"
  std::vector<std::string> parts{first, last};
  for (auto part : parts) {
    if ( part.length() == 0 ) {
      throw std::runtime_error{"LinearExpression::Empty term in expression"};
    }
    if ( part[0] == '-' ) {
      SIGN *= -1.0;
      part.erase(0,1);
    }

    Attribute* variable = nullptr;
    for ( auto &[key, attribute] : attributeMap ) {
      size_t pos = part.find(key);
      if ( pos != std::string::npos ) {
        if ( attribute->type == ValueType::STRING || attribute->type == ValueType::COLLECTION ) {
          throw std::runtime_error("LinearExpression: non-numeric variable '" + attribute->name + "'");
        }
        variable = attribute;
        part.erase(pos,key.length()); // remove attribute name from part
        part.erase(remove(part.begin(), part.end(), '*'), part.end());
        break;
      }
    }

    if ( part.length() == 0 ) {
      // use factor 1 if not explicitly provided
      part = "1";
    }
    else if ( part == Keyword::True ) {
      part = "1";
    }
    else if ( part == Keyword::False ) {
      part = "0";
    }

    size_t pos = part.find("/");
    if ( pos == std::string::npos ) {
      terms.push_back({ SIGN * NumericType(stod(part)), variable });
    }
    else {
      if ( pos == 0 ) {
        // make sure that there is a number before the division sign
        part = std::string("1") + part;
        pos++;
      }
      terms.push_back({ SIGN * NumericType(stod(part.substr(0,pos)) / stod(part.substr(pos+1))), variable });      
    } 
  }
}

void LinearExpression::parseInequality(const std::string& comparisonOperator) {
  auto expressions = strutil::split(expression, comparisonOperator);
  if ( expressions.size() != 2 ) {
    throw std::runtime_error("LinearExpression: (in-)equality must have l.h.s. and r.h.s.");
  }
  parse( expressions.front() );
  parse( expressions.back(), -1 );
}

std::optional<BPMNOS::number> LinearExpression::execute(const Values& values) const {
  NumericType result = 0;
  for ( auto& [coefficient,variable] : terms ) {
    if ( variable ) {
      if ( !values[variable->index].has_value() ) {
        // return undefined due to missing variable 
        return std::nullopt;
      }
      result += coefficient * values[variable->index].value();
    }
    else {
      result += coefficient;
    }
  }
  
  if ( type == Type::DEFAULT ) {
    return number(result);
  }
  else if ( type == Type::EQUAL ) {
    return BPMNOS::to_number( (number(result) == 0), BOOLEAN);
  }
  else if ( type == Type::NOTEQUAL ) {
    return BPMNOS::to_number( (number(result) != 0), BOOLEAN);
  }
  else if ( type == Type::GREATEROREQUAL ) {
    return BPMNOS::to_number( (number(result) >= 0), BOOLEAN);
  }
  else if ( type == Type::GREATERTHAN ) {
    return BPMNOS::to_number( (number(result) > 0), BOOLEAN);
  }
  else if ( type == Type::LESSOREQUAL ) {
    return BPMNOS::to_number( (number(result) <= 0), BOOLEAN);
  }
  else if ( type == Type::LESSTHAN ) {
    return BPMNOS::to_number( (number(result) < 0), BOOLEAN);
  }
  
  return std::nullopt;
}

std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > LinearExpression::getBounds(const Attribute* attribute, const Values& values) const {
  if ( type == Type::DEFAULT || type == Type::NOTEQUAL ) {
    return {std::nullopt,std::nullopt};
  }

  NumericType result = 0;
  NumericType denominator = 0;
  
  for ( auto& [coefficient,variable] : terms ) {
    if ( variable ) {
      if ( variable == attribute ) {
        denominator = -coefficient;
      }
      else {
        if ( !values[variable->index].has_value() ) {
          // return no bounds due to missing variable 
          return {std::nullopt,std::nullopt};
        }
        result += coefficient * values[variable->index].value();
      }
    }
    else {
      result += coefficient;
    }
  }

  if ( denominator == 0 ) {
    return {std::nullopt,std::nullopt};
  }

  if ( type == Type::EQUAL ) {
    if ( attribute->type == DECIMAL ) {
      return { BPMNOS::to_number( (number(result)/denominator), DECIMAL), BPMNOS::to_number( (number(result)/denominator), DECIMAL) };
    }
    else {
      return { BPMNOS::to_number( ceil(number(result)/denominator), DECIMAL), BPMNOS::to_number( floor(number(result)/denominator), DECIMAL) };
    }
  }
  else if ( type == Type::GREATEROREQUAL ) {
    if ( denominator > 0 ) {
      if ( attribute->type == DECIMAL ) {
        return { BPMNOS::to_number( (number(result)/denominator), DECIMAL), std::nullopt };
      }
      else {
        return { BPMNOS::to_number( ceil(number(result)/denominator), DECIMAL), std::nullopt };
      }
    }
    else {
      if ( attribute->type == DECIMAL ) {
        return { std::nullopt, BPMNOS::to_number( (number(result)/denominator), DECIMAL) };
      }
      else {
        return { std::nullopt, BPMNOS::to_number( floor(number(result)/denominator), DECIMAL) };
      }
    }
  }
  else if ( type == Type::GREATERTHAN ) {
    if ( denominator > 0 ) {
      if ( attribute->type == DECIMAL ) {
        return { BPMNOS::to_number( (number(result)/denominator), DECIMAL) + BPMNOS_NUMBER_PRECISION, std::nullopt };
      }
      else {
        return { BPMNOS::to_number( floor(number(result)/denominator), INTEGER) + 1, std::nullopt };
      }
    }
    else {
      if ( attribute->type == DECIMAL ) {
        return { std::nullopt, BPMNOS::to_number( (number(result)/denominator) , DECIMAL) - BPMNOS_NUMBER_PRECISION };
      }
      else {
        return { std::nullopt, BPMNOS::to_number( ceil(number(result)/denominator) , DECIMAL) - 1 };
      }
    }
  }
  else if ( type == Type::LESSOREQUAL ) {
    if ( denominator > 0 ) {
      if ( attribute->type == DECIMAL ) {
        return { std::nullopt, BPMNOS::to_number( (number(result)/denominator), DECIMAL) };
      }
      else {
        return { std::nullopt, BPMNOS::to_number( floor(number(result)/denominator), DECIMAL) };
      }
    }
    else {
      if ( attribute->type == DECIMAL ) {
        return { BPMNOS::to_number( (number(result)/denominator), DECIMAL), std::nullopt };
      }
      else {
        return { BPMNOS::to_number( ceil(number(result)/denominator), DECIMAL), std::nullopt };
      }
    }
  }
  else if ( type == Type::LESSTHAN ) {
    if ( denominator > 0 ) {
      if ( attribute->type == DECIMAL ) {
        return { std::nullopt, BPMNOS::to_number( (number(result)/denominator), DECIMAL) - BPMNOS_NUMBER_PRECISION};
      }
      else {
        return { std::nullopt, BPMNOS::to_number( ceil(number(result)/denominator), DECIMAL) - 1};
      }
    }
    else {
      if ( attribute->type == DECIMAL ) {
        return { BPMNOS::to_number( (number(result)/denominator), DECIMAL) + BPMNOS_NUMBER_PRECISION, std::nullopt };
      }
      else {
        return { BPMNOS::to_number( floor(number(result)/denominator), DECIMAL) + 1, std::nullopt };
      }
    }
  }
  return {std::nullopt,std::nullopt};
}

