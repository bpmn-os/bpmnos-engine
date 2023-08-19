#include "Parameter.h"

using namespace BPMNOS;

Parameter::Parameter(XML::bpmnos::tParameter* parameter, AttributeMap& attributeMap)
  : element(parameter)
  , name(parameter->name.value.value)
  , attribute(getAttribute(attributeMap))
  , value(parameter->value ? std::optional< std::reference_wrapper<XML::Value> >(parameter->value->get().value) : std::nullopt )
{
}

std::optional< std::reference_wrapper<Attribute> > Parameter::getAttribute(AttributeMap& attributeMap) {
  if ( element->attribute.has_value() ) {
    try {
      return *attributeMap.at(element->attribute->get().value);
    }
    catch ( ... ){
      throw std::runtime_error("Parameter: illegal attribute for parameter '" + name + "'");
    }
  }
  return std::nullopt;  
}
