#include "Set.h"
#include "model/parser/src/extensionElements/Operator.h"

using namespace BPMNOS;

Set::Set(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
  try {
    parameter = base->parameterMap.at("set").get();
  }
  catch ( ... ){
    throw std::runtime_error("Set: required parameter 'set' not provided for operator " + base->id + "'");
  }

}

