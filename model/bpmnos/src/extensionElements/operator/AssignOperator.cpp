#include "AssignOperator.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

using namespace BPMNOS::Model;

AssignOperator::AssignOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : Operator(operator_, attributeRegistry)
{
  try {
    parameter = parameterMap.at("assign").get();
    if ( parameter->attribute.has_value() ) {
      inputs.insert( &parameter->attribute->get() );
    }
    
  }
  catch ( ... ){
    throw std::runtime_error("Assign: required parameter 'assign' not provided for operator " + id + "'");
  }
}

template <typename DataType>
void AssignOperator::_apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  if ( parameter->attribute.has_value() && status[parameter->attribute->get().index].has_value() ) {
    // Assign value to value of given attribute (if defined)
    attributeRegistry.setValue( attribute, status, data, globals, attributeRegistry.getValue(&parameter->attribute->get(), status, data, globals) );
  }
  else if ( parameter->value.has_value() ) {
    // Assign value to parameter value (if defined)
    attributeRegistry.setValue( attribute, status, data, globals, to_number( parameter->value->get().value, attribute->type ) );
  }
  else {
    // Assign value to undefined if no attribute with value is given and no explicit value is given
    attributeRegistry.setValue( attribute, status, data, globals, std::nullopt);
  }
}

template void AssignOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void AssignOperator::_apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;

