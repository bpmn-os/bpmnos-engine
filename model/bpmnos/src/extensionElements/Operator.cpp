#include "Operator.h"
#include "operator/UnassignOperator.h"
#include "operator/AssignOperator.h"
#include "operator/LookupOperator.h"
#include "operator/ExpressionOperator.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, const AttributeMap& statusAttributes)
  : element(operator_)
  , id(operator_->id.value.value)
  , attribute(statusAttributes.at(operator_->attribute.value))
  , statusAttributes(statusAttributes)
{
  attribute->isImmutable = false;

  for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
    parameterMap[parameter.name.value] = std::make_unique<Parameter>(&parameter,statusAttributes);
  }
}

std::unique_ptr<Operator> Operator::create(XML::bpmnos::tOperator* operator_, AttributeMap& statusAttributes) {
  std::string& operatorType = operator_->getRequiredAttributeByName("type").value.value;
  if ( operatorType == "unassign" ) {
    return std::make_unique<UnassignOperator>(operator_, statusAttributes);
  }
  else if ( operatorType == "assign" ) {
    return std::make_unique<AssignOperator>(operator_, statusAttributes);
  }
  else if ( operatorType == "lookup" ) {
    return std::make_unique<LookupOperator>(operator_, statusAttributes);
  }
  else if ( operatorType == "expression" ) {
    return std::make_unique<ExpressionOperator>(operator_, statusAttributes);
  }
  throw std::logic_error("Operator: illegal operator type '" + operatorType + "' for operator '" + operator_->id.value.value + "'");
}

