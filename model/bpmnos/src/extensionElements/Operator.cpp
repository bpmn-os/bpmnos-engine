#include "Operator.h"
#include "operator/UnassignOperator.h"
#include "operator/AssignOperator.h"
#include "operator/LookupOperator.h"
#include "operator/ExpressionOperator.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : element(operator_)
  , id(operator_->id.value.value)
  , attribute(attributeRegistry[operator_->attribute.value])
  , attributeRegistry(attributeRegistry)
{
  attribute->isImmutable = false;
    
  for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
    parameterMap[parameter.name.value] = std::make_unique<Parameter>(&parameter,attributeRegistry);
  }
}

std::unique_ptr<Operator> Operator::create(XML::bpmnos::tOperator* operator_, AttributeRegistry& attributeRegistry) {
  std::string& operatorType = operator_->getRequiredAttributeByName("type").value.value;
  if ( operatorType == "unassign" ) {
    return std::make_unique<UnassignOperator>(operator_, attributeRegistry);
  }
  else if ( operatorType == "assign" ) {
    return std::make_unique<AssignOperator>(operator_, attributeRegistry);
  }
  else if ( operatorType == "lookup" ) {
    return std::make_unique<LookupOperator>(operator_, attributeRegistry);
  }
  else if ( operatorType == "expression" ) {
    return std::make_unique<ExpressionOperator>(operator_, attributeRegistry);
  }
  throw std::logic_error("Operator: illegal operator type '" + operatorType + "' for operator '" + operator_->id.value.value + "'");
}

