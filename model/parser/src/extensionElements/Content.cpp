#include "Content.h"

using namespace BPMNOS;

Content::Content(XML::bpmnos::tContent* content, AttributeMap& attributeMap)
  : element(content)
  , id(content->id.value.value)
  , key(content->key.value.value)
  , attribute(getAttribute(attributeMap))
  , type(attribute ? attribute->get().type : ValueType::STRING )
{
  if ( content->value.has_value() ) {
    value = to_number( content->value->get().value.value, type ); 
  }

}

std::optional< std::reference_wrapper<Attribute> > Content::getAttribute(AttributeMap& attributeMap) {
  if ( element->attribute.has_value() ) {
    if ( auto attributeIt = attributeMap.find(element->attribute->get().value); attributeIt != attributeMap.end() ) {
      return *attributeIt->second;
    }
    else {
      throw std::runtime_error("Content: illegal attribute for content '" + id + "'");
    }
  }
  return std::nullopt;  
}
