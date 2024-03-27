#include "NullCondition.h"
#include "model/utility/src/Keywords.h"
#include <regex>
#include <strutil.h>

using namespace BPMNOS::Model;

NullCondition::NullCondition(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : Expression(parameter, attributeRegistry)
  , attribute(nullptr)
{
  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("NullCondition: expression must be given by value");
  }

  if ( strutil::contains(expression, "==") ) {
    type = Type::ISNULL;
    parse("==");
  }
  else if ( strutil::contains(expression, "!=") ) {
    type = Type::NOTNULL;
    parse("!=");
  }
  else {
    throw std::runtime_error("NullCondition: invalid or missing comparison operator");
  }
}

void NullCondition::parse(const std::string& comparisonOperator) {
  auto expressions = strutil::split(expression, comparisonOperator);
  if ( expressions.size() != 2 ) {
    throw std::runtime_error("NullCondition: condition must have l.h.s. and r.h.s.");
  }
  std::string lhs = strutil::trim_copy(expressions.front());
  std::string rhs = strutil::trim_copy(expressions.back());

  attribute = attributeRegistry[lhs];

  if ( rhs != Keyword::Undefined ) {
    throw std::runtime_error("NullCondition: cannot find " + Keyword::Undefined + " keyword in r.h.s.");
  }


}


template <typename DataType>
std::optional<BPMNOS::number> NullCondition::_execute(const BPMNOS::Values& status, const DataType& data) const {
  auto value = attributeRegistry.getValue(attribute,status,data);
  if ( type == Type::ISNULL ) {
    return BPMNOS::to_number( !value.has_value() , BOOLEAN);
  }
  else if ( type == Type::NOTNULL ) {
    return BPMNOS::to_number( value.has_value() , BOOLEAN);
  } 
  return std::nullopt;
}

template std::optional<BPMNOS::number> NullCondition::_execute<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data) const;
template std::optional<BPMNOS::number> NullCondition::_execute<BPMNOS::Globals>(const BPMNOS::Values& status, const BPMNOS::Globals& data) const;

