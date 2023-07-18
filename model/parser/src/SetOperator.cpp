#include "SetOperator.h"
#include "Operator.h"

using namespace BPMNOS;

SetOperator::SetOperator(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
  try {
    parameter = base->parameterMap.at("set").get();
  }
  catch ( ... ){
    throw std::runtime_error("SetOperator: required parameter 'set' not provided for operator " + base->id + "'");
  }

}

