#include "UnassignOperator.h"

using namespace BPMNOS::Model;

UnassignOperator::UnassignOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : Operator(operator_, attributeRegistry)
{
}

template <typename DataType>
void UnassignOperator::_apply(BPMNOS::Values& status, DataType& data) const {
  attributeRegistry.setValue( attribute, status, data, std::nullopt);
}

template void UnassignOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data) const;
template void UnassignOperator::_apply<BPMNOS::Globals>(BPMNOS::Values& status, BPMNOS::Globals& data) const;

