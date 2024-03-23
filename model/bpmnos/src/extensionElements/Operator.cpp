#include "Operator.h"
#include "operator/UnassignOperator.h"
#include "operator/AssignOperator.h"
#include "operator/LookupOperator.h"
#include "operator/ExpressionOperator.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, const AttributeMap& attributeMap)
  : element(operator_)
  , id(operator_->id.value.value)
  , attribute(attributeMap.at(operator_->attribute.value))
  , attributeMap(attributeMap)
{
  attribute->isImmutable = false;

  for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
    parameterMap[parameter.name.value] = std::make_unique<Parameter>(&parameter,attributeMap);
  }
}

std::unique_ptr<Operator> Operator::create(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap) {
  std::string& operatorType = operator_->getRequiredAttributeByName("type").value.value;
  if ( operatorType == "unassign" ) {
    return std::make_unique<UnassignOperator>(operator_, attributeMap);
  }
  else if ( operatorType == "assign" ) {
    return std::make_unique<AssignOperator>(operator_, attributeMap);
  }
  else if ( operatorType == "lookup" ) {
    return std::make_unique<LookupOperator>(operator_, attributeMap);
  }
  else if ( operatorType == "expression" ) {
    return std::make_unique<ExpressionOperator>(operator_, attributeMap);
  }
  throw std::logic_error("Operator: illegal operator type '" + operatorType + "' for operator '" + operator_->id.value.value + "'");
}

