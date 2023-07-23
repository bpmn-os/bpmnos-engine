#include "Content.h"

using namespace BPMNOS;

Content::Content(XML::bpmnos::tContent* content, AttributeMap& attributeMap)
  : element(content)
  , id(content->id.value)
  , key(content->key.value)
  , attribute(getAttribute(attributeMap))
  , value(content->value)
{
}

std::optional< std::reference_wrapper<Attribute> > Content::getAttribute(AttributeMap& attributeMap) {
  if ( element->attribute.has_value() ) {
    try {
      return *attributeMap.at(element->attribute->get());
    }
    catch ( ... ){
      throw std::runtime_error("Content: illegal attribute for content '" + id + "'");
    }
  }
  return std::nullopt;  
}
