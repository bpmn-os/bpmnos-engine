#include "LinearExpression.h"
#include "model/utility/src/Keywords.h"
#include <regex>

using namespace BPMNOS;

LinearExpression::LinearExpression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Expression(operator_, attributeMap)
{
  if ( attribute->type == ValueType::STRING ) {
    throw std::runtime_error("LinearExpression: non-numeric result of operator '" + id + "'");
  }

  parameter = parameterMap.at("linear").get();

  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("LinearExpression: expression of operator '" + id + "' must be given by value");
  }

  std::string expression = parameter->value.value().get().value;
 
  expression.erase(remove(expression.begin(), expression.end(), ' '), expression.end()); // remove all spaces
  expression = std::regex_replace(expression, std::regex("-"), "+-"); // replace "-" by "+-"
  std::regex re1("[+]");
  std::sregex_token_iterator first{expression.begin(), expression.end(), re1, -1}, last; // split by "+"
  std::vector<std::string> parts{first, last};
  for (auto part : parts) {
    if ( part.length() == 0 ) {
      throw std::runtime_error{"LinearExpression::Empty term in expression"};
    }
    NumericType SIGN = 1.0;
    if ( part[0] == '-' ) {
      SIGN = -1.0;
      part.erase(0,1);
    }

    Attribute* variable = nullptr;
    for ( auto &[key, value] : attributeMap ) {
      size_t pos = part.find(key);
      if ( pos != std::string::npos ) {
        variable = value;
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

void LinearExpression::apply(Values& status) const {
  NumericType result = 0;
  for ( auto& [coefficient,variable] : terms ) {
    if ( variable ) {
      if ( !status[variable->index].has_value() ) {
        // return undefined due to missing variable 
        status[attribute->index] = std::nullopt;
        return;
      }
      result += coefficient * status[variable->index].value();
    }
    else {
      result += coefficient;
    }
  }
  status[attribute->index] = number(result);
}

