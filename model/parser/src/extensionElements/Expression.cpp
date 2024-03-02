#include "Expression.h"
#include "model/parser/src/extensionElements/expression/LinearExpression.h"
#include "model/parser/src/extensionElements/expression/GenericExpression.h"
#include "model/parser/src/extensionElements/expression/NullCondition.h"

using namespace BPMNOS::Model;

Expression::Expression(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap)
  : attributeMap(attributeMap)
  , expression(parameter->value.value().get().value)
{
}

std::unique_ptr<Expression> Expression::create(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap) {
  if ( parameter->name.value.value == "linear" ) {
    return std::make_unique<LinearExpression>(parameter, attributeMap);
  }
  else if ( parameter->name.value.value == "generic" ) {
    return std::make_unique<GenericExpression>(parameter, attributeMap);
  }
  else if ( parameter->name.value.value == "nullcondition" ) {
    return std::make_unique<NullCondition>(parameter, attributeMap);
  }
  else {
    throw std::logic_error("Expression: illegal expression type");
  }
}

