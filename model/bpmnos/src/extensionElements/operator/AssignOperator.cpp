#include "AssignOperator.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

using namespace BPMNOS::Model;

AssignOperator::AssignOperator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
  try {
    parameter = parameterMap.at("assign").get();
  }
  catch ( ... ){
    throw std::runtime_error("Assign: required parameter 'assign' not provided for operator " + id + "'");
  }
}

void AssignOperator::apply(Values& status) const {
  if ( parameter->attribute.has_value() && status[parameter->attribute->get().index].has_value() ) {
    // Assign value to value of given attribute (if defined)
    status[attribute->index] = status[parameter->attribute->get().index];
  }
  else if ( parameter->value.has_value() ) {
    // Assign value to parameter value (if defined)
    status[attribute->index] = to_number( parameter->value->get().value, attribute->type );
  }
  else {
    // Assign value to undefined if no attribute with value is given and no explicit value is given
    status[attribute->index] = std::nullopt;
  }
}

