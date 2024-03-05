#include "StringExpression.h"
#include "model/utility/src/Keywords.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

StringExpression::StringExpression(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap)
  : Expression(parameter, attributeMap)
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

  if ( auto it = attributeMap.find(parts.front());
    it != attributeMap.end()
  ) {
    attribute = it->second;
  }
  else {
    throw std::runtime_error("StringExpression: cannot find attribute");
  }

  if ( strutil::starts_with(parts.back(),"\"") && strutil::ends_with(parts.back(),"\"") ) {
    rhs = BPMNOS::to_number( parts.back().substr(1,parts.back().size()-2), STRING );
  }
  else {
    if ( auto it = attributeMap.find(parts.back());
      it != attributeMap.end()
    ) {
      rhs = it->second;
    }
    else {
      throw std::runtime_error("StringExpression: cannot find attribute holding comparison string");
   }
  }
}

std::optional<BPMNOS::number> StringExpression::execute(const Values& values) const {
  bool equals;
  if ( std::holds_alternative<const Attribute*>(rhs) ) {
    auto other = std::get<const Attribute *>(rhs);
    equals = ( values[attribute->index] == values[other->index] );
  }
  else {
    equals = ( values[attribute->index].has_value() && values[attribute->index].value() == std::get<BPMNOS::number>(rhs) );
  }
  
  if ( type == Type::EQUAL ) {
    return BPMNOS::to_number( equals, BOOLEAN);
  }
  else if ( type == Type::NOTEQUAL ) {
    return BPMNOS::to_number( !equals, BOOLEAN);
  }
  
  return std::nullopt;
}
