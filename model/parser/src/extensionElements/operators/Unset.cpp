#include "Unset.h"

using namespace BPMNOS;

Unset::Unset(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
}

void Unset::apply(Values& status) const {
  status[attribute->index] = std::nullopt;
}

