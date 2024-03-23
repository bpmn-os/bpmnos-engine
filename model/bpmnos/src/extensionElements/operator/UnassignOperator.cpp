#include "UnassignOperator.h"

using namespace BPMNOS::Model;

UnassignOperator::UnassignOperator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
}

void UnassignOperator::apply(Values& status) const {
  status[attribute->index] = std::nullopt;
}

