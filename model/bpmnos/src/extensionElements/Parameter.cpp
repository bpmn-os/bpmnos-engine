#include "Parameter.h"

using namespace BPMNOS::Model;

Parameter::Parameter(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : element(parameter)
  , name(parameter->name.value.value)
  , attribute(getAttribute(attributeRegistry))
  , value(parameter->value ? std::optional< std::reference_wrapper<XML::Value> >(parameter->value->get().value) : std::nullopt )
{
}

std::optional< std::reference_wrapper<Attribute> > Parameter::getAttribute(const AttributeRegistry& attributeRegistry) const {
  if ( element->attribute.has_value() ) {
    return *attributeRegistry[element->attribute->get().value];
  }
  return std::nullopt;  
}
