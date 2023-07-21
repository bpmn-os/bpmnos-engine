#include "Unset.h"
#include "model/parser/src/Operator.h"

using namespace BPMNOS;

Unset::Unset(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
}

