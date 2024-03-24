#include "UnassignOperator.h"

using namespace BPMNOS::Model;

UnassignOperator::UnassignOperator(XML::bpmnos::tOperator* operator_, AttributeMap& statusAttributes)
  : Operator(operator_, statusAttributes)
{
}

void UnassignOperator::apply(Values& status) const {
  status[attribute->index] = std::nullopt;
}

