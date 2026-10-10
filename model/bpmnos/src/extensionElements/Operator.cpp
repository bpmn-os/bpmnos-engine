#include "Operator.h"
#include "model/utility/src/InputEncoder.h"

using namespace BPMNOS::Model;

Operator::Operator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : element(operator_)
  , id(operator_->id.value.value)
  , expression(Expression(InputEncoder(operator_->expression.value.value),attributeRegistry))
  , attributeRegistry(attributeRegistry)
  , attribute(getAttribute())
{
  attribute->isImmutable = false;
}

template <typename DataType>
BPMNOS::number Operator::apply(BPMNOS::Status& status, DataType& data) const {
  return attributeRegistry.setValue( attribute, status, data, BPMNOS::to_value( expression.execute(status,data) ) );
}

template BPMNOS::number Operator::apply<BPMNOS::Data>(BPMNOS::Status& status, BPMNOS::Data& data) const;
template BPMNOS::number Operator::apply<BPMNOS::SharedData>(BPMNOS::Status& status, BPMNOS::SharedData& data) const;

Attribute* Operator::getAttribute() const {
  if ( auto& name = expression.compiled.getTarget(); name.has_value() ) {
    return attributeRegistry[ name.value() ];
  }
  throw std::runtime_error("Operator: expression is not an assignment");
}

