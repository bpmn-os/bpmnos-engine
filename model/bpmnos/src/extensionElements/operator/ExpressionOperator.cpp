#include "ExpressionOperator.h"

using namespace BPMNOS::Model;

ExpressionOperator::ExpressionOperator(XML::bpmnos::tOperator* operator_, const AttributeMap& statusAttributes)
  : Operator(operator_, statusAttributes)
  , expression(Expression::create( &operator_->getRequiredChild<XML::bpmnos::tParameter>(), statusAttributes))
{
  if ( attribute->type == ValueType::STRING || attribute->type == ValueType::COLLECTION ) {
    throw std::runtime_error("ExpressionOperator: non-numeric result of expression operator '" + id + "'");
  }
}

void ExpressionOperator::apply(Values& values) const {
  values[attribute->index] = expression->execute(values);
}
