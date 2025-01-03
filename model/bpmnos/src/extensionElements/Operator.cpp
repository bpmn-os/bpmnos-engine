#include "Operator.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : element(operator_)
  , id(operator_->id.value.value)
  , expression(Expression(operator_->expression.value.value,attributeRegistry))
  , attributeRegistry(attributeRegistry)
  , attribute(getAttribute())
  , inputs(expression.inputs)
{
  attribute->isImmutable = false;
}

template <typename DataType>
void Operator::apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  attributeRegistry.setValue( attribute, status, data, globals, expression.execute(status,data,globals) );
}

template void Operator::apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void Operator::apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;

Attribute* Operator::getAttribute() const {
  if ( auto& name = expression.compiled.getTarget(); name.has_value() ) {
    return attributeRegistry[ name.value() ];
  }
  throw std::runtime_error("Operator: expression is not an assignment");

}

