#include "Unset.h"
#include "model/parser/src/extensionElements/Operator.h"

using namespace BPMNOS;

Unset::Unset(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
}

void Unset::execute(Values& status) const {
  status[attribute->index] = std::nullopt;
}

