#include "Unassign.h"

using namespace BPMNOS::Model;

Unassign::Unassign(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
}

void Unassign::apply(Values& status) const {
  status[attribute->index] = std::nullopt;
}

