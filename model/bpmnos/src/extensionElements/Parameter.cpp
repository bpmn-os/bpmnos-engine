#include "Parameter.h"

using namespace BPMNOS::Model;

Parameter::Parameter(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes)
  : element(parameter)
  , name(parameter->name.value.value)
  , attribute(getAttribute(statusAttributes))
  , value(parameter->value ? std::optional< std::reference_wrapper<XML::Value> >(parameter->value->get().value) : std::nullopt )
{
}

std::optional< std::reference_wrapper<Attribute> > Parameter::getAttribute(const AttributeMap& statusAttributes) {
  if ( element->attribute.has_value() ) {
    try {
      return *statusAttributes.at(element->attribute->get().value);
    }
    catch ( ... ){
      throw std::runtime_error("Parameter: illegal attribute for parameter '" + name + "'");
    }
  }
  return std::nullopt;  
}
