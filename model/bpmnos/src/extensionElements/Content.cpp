#include "Content.h"

using namespace BPMNOS::Model;

Content::Content(XML::bpmnos::tContent* content, AttributeMap& statusAttributes)
  : element(content)
  , id(content->id.value.value)
  , key(content->key.value.value)
  , attribute(getAttribute(statusAttributes))
{
  if ( content->value.has_value() && content->value->get().value.value.size() ) {
    value = content->value->get().value.value; 
  }
}

std::optional< std::reference_wrapper<Attribute> > Content::getAttribute(AttributeMap& statusAttributes) {
  if ( element->attribute.has_value() ) {
    if ( auto attributeIt = statusAttributes.find(element->attribute->get().value); attributeIt != statusAttributes.end() ) {
      return *attributeIt->second;
    }
    else {
      throw std::runtime_error("Content: illegal attribute for content '" + id + "'");
    }
  }
  return std::nullopt;  
}
