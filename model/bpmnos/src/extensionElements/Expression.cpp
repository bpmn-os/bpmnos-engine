#include "Expression.h"
#include "model/bpmnos/src/extensionElements/expression/LinearExpression.h"
#include "model/bpmnos/src/extensionElements/expression/GenericExpression.h"
#include "model/bpmnos/src/extensionElements/expression/NullCondition.h"
#include "model/bpmnos/src/extensionElements/expression/Enumeration.h"
#include "model/bpmnos/src/extensionElements/expression/StringExpression.h"

using namespace BPMNOS::Model;

Expression::Expression(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes)
  : statusAttributes(statusAttributes)
  , expression(parameter->value.value().get().value)
{
}

std::unique_ptr<Expression> Expression::create(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes) {
  if ( parameter->name.value.value == "linear" ) {
    return std::make_unique<LinearExpression>(parameter, statusAttributes);
  }
  else if ( parameter->name.value.value == "generic" ) {
    return std::make_unique<GenericExpression>(parameter, statusAttributes);
  }
  else if ( parameter->name.value.value == "nullcondition" ) {
    return std::make_unique<NullCondition>(parameter, statusAttributes);
  }
  else if ( parameter->name.value.value == "enumeration" ) {
    return std::make_unique<Enumeration>(parameter, statusAttributes);
  }
  else if ( parameter->name.value.value == "string" ) {
    return std::make_unique<StringExpression>(parameter, statusAttributes);
  }
  else {
    throw std::logic_error("Expression: illegal expression type");
  }
}

std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > Expression::getBounds([[maybe_unused]] const Attribute* attribute, [[maybe_unused]] const Values& values) const {
  return {std::nullopt,std::nullopt};
}

