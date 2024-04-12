#include "StringExpression.h"
#include "model/utility/src/Keywords.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

StringExpression::StringExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : Expression(parameter, attributeRegistry)
{
  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("StringExpression: expression must be given by value");
  }

  std::vector<std::string> parts;
  
  if ( strutil::contains(expression, "==") ) {
    type = Type::EQUAL;
    parts = strutil::split(expression, "==");
  }
  else if ( strutil::contains(expression, "!=") ) {
    type = Type::NOTEQUAL;
    parts = strutil::split(expression, "!=");
  }
  
  if ( parts.size() != 2 ) {
    throw std::runtime_error("StringExpression: (in-)equality must have l.h.s. and r.h.s.");
  }

  strutil::trim(parts.front());
  strutil::trim(parts.back());

  attribute = attributeRegistry[parts.front()];
  inputs.insert( attribute );

  if ( strutil::starts_with(parts.back(),"\"") && strutil::ends_with(parts.back(),"\"") ) {
    // right hand side with quotes is a string
    rhs = BPMNOS::to_number( parts.back().substr(1,parts.back().size()-2), STRING );
  }
  else {
    // right hand side without quotes is an attribute name
    rhs = attributeRegistry[parts.back()];
    inputs.insert( std::get<const Attribute *>(rhs) );
  }
}

template <typename DataType>
std::optional<BPMNOS::number> StringExpression::_execute(const BPMNOS::Values& status, const DataType& data) const {
  bool equals;
  if ( std::holds_alternative<const Attribute*>(rhs) ) {
    auto other = std::get<const Attribute *>(rhs);
    equals = ( attributeRegistry.getValue(attribute,status,data) == attributeRegistry.getValue(other,status,data) );
  }
  else {
    auto value = attributeRegistry.getValue(attribute,status,data);
    equals = ( value.has_value() && value.value() == std::get<BPMNOS::number>(rhs) );
  }
  
  if ( type == Type::EQUAL ) {
    return BPMNOS::to_number( equals, BOOLEAN);
  }
  else if ( type == Type::NOTEQUAL ) {
    return BPMNOS::to_number( !equals, BOOLEAN);
  }
  
  return std::nullopt;
}

template std::optional<BPMNOS::number> StringExpression::_execute<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data) const;
template std::optional<BPMNOS::number> StringExpression::_execute<BPMNOS::Globals>(const BPMNOS::Values& status, const BPMNOS::Globals& data) const;

