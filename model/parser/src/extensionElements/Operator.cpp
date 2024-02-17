#include "Operator.h"
#include "operators/Unassign.h"
#include "operators/Assign.h"
#include "operators/Lookup.h"
#include "operators/Expression.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
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
    return std::make_unique<Unassign>(operator_, attributeMap);
  }
  else if ( operatorType == "assign" ) {
    return std::make_unique<Assign>(operator_, attributeMap);
  }
  else if ( operatorType == "lookup" ) {
    return std::make_unique<Lookup>(operator_, attributeMap);
  }
  else if ( operatorType == "expression" ) {
    return Expression::create(operator_, attributeMap);
  }
  throw std::logic_error("Operator: illegal operator type '" + operatorType + "' for operator '" + operator_->id.value.value + "'");
}

