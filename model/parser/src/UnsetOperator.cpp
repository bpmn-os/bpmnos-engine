#include "UnsetOperator.h"
#include "Operator.h"

using namespace BPMNOS;

UnsetOperator::UnsetOperator(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
}

