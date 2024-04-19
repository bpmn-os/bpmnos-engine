#include "UnassignOperator.h"

using namespace BPMNOS::Model;

UnassignOperator::UnassignOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : Operator(operator_, attributeRegistry)
{
}

template <typename DataType>
void UnassignOperator::_apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  attributeRegistry.setValue( attribute, status, data, globals, std::nullopt);
}

template void UnassignOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void UnassignOperator::_apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;

