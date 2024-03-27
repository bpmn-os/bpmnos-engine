#include "Content.h"

using namespace BPMNOS::Model;

Content::Content(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry)
  : element(content)
  , id(content->id.value.value)
  , key(content->key.value.value)
  , attribute(getAttribute(attributeRegistry))
{
  if ( content->value.has_value() && content->value->get().value.value.size() ) {
    value = content->value->get().value.value; 
  }
}

std::optional< std::reference_wrapper<Attribute> > Content::getAttribute(const AttributeRegistry& attributeRegistry) const {
  if ( element->attribute.has_value() ) {
    return std::ref(*attributeRegistry[element->attribute->get().value]);
  }
  return std::nullopt;  
}
