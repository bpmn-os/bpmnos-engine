#include "Expression.h"
#include "LinearExpression.h"
#include "GenericExpression.h"

using namespace BPMNOS::Model;

Expression::Expression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
}

std::unique_ptr<Expression> Expression::create(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap) {
  auto parameters = operator_->getChildren<XML::bpmnos::tParameter>();

  auto findParameterByName = [](const std::vector< std::reference_wrapper<XML::bpmnos::tParameter> >& parameters, const std::string& name) {
    return find_if(
      parameters.begin(),
      parameters.end(),
      [&name](const XML::bpmnos::tParameter& parameter) {
        return parameter.name.value.value == name;
      }
    );
  };

  if ( auto parameter = findParameterByName(parameters, "linear"); parameter != parameters.end() ) {
    return std::make_unique<LinearExpression>(operator_, attributeMap);
  }
  if ( auto parameter = findParameterByName(parameters, "generic"); parameter != parameters.end() ) {
    return std::make_unique<GenericExpression>(operator_, attributeMap);
  }
  throw std::logic_error("Expression: illegal expression for operator '" + operator_->id.value.value + "'");
}

