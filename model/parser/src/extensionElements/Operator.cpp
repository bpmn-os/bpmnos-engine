#include "Operator.h"
#include "operators/Unset.h"
#include "operators/Set.h"
#include "operators/Lookup.h"
#include "operators/Expression.h"

using namespace BPMNOS;

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
  if ( operatorType == "unset" ) {
    return std::make_unique<Unset>(operator_, attributeMap);
  }
  else if ( operatorType == "set" ) {
    return std::make_unique<Set>(operator_, attributeMap);
  }
  else if ( operatorType == "lookup" ) {
    return std::make_unique<Lookup>(operator_, attributeMap);
  }
  throw std::logic_error("Operator: illegal operator type '" + operatorType + "' for operator '" + operator_->id.value.value + "'");
}

