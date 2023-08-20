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

void Set::execute(Values& status) const {
  if ( parameter->attribute.has_value() && status[parameter->attribute->get().index].has_value() ) {
    // set value to value of given attribute (if defined)
    status[attribute->index] = status[parameter->attribute->get().index];
  }
  else if ( parameter->value.has_value() ) {
    // set value to parameter value (if defined)
    status[attribute->index] = to_number( parameter->value->get().value, attribute->type );
  }
  else {
    // set value to undefined if no attribute with value is given and no explicit value is given
    status[attribute->index] = std::nullopt;
  }
}

