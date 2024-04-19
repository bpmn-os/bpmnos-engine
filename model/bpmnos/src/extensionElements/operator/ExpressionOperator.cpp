#include "ExpressionOperator.h"

using namespace BPMNOS::Model;

ExpressionOperator::ExpressionOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : Operator(operator_, attributeRegistry)
  , expression(Expression::create( &operator_->getRequiredChild<XML::bpmnos::tParameter>(), attributeRegistry))

{
  if ( attribute->type == ValueType::STRING || attribute->type == ValueType::COLLECTION ) {
    throw std::runtime_error("ExpressionOperator: non-numeric result of expression operator '" + id + "'");
  }
  inputs = expression->inputs;
}

template <typename DataType>
void ExpressionOperator::_apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  attributeRegistry.setValue( attribute, status, data, globals, expression->execute(status,data,globals) );
}

template void ExpressionOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void ExpressionOperator::_apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;


