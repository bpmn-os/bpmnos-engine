#include "Operator.h"

using namespace BPMNOS;

Operator::Operator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : element(operator_)
  , id(operator_->id.value)
  , attribute(attributeMap.at(operator_->attribute))
  , attributeMap(attributeMap)
{
  for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
    parameterMap[parameter.name.value] = std::make_unique<Parameter>(&parameter,attributeMap);
  }

  // set implementation to the specific operator
  std::string& operatorType = element->getRequiredAttributeByName("type").value;
  if ( operatorType == "set" ) {
    implementation = std::make_unique<SetOperator>(this,attribute);
  }
  else if ( operatorType == "unset" ) {
    implementation = std::make_unique<UnsetOperator>(this,attribute);
  }

}

