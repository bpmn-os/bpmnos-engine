#include "Content.h"

using namespace BPMNOS::Model;

Content::Content(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry)
  : element(content)
  , key(content->key.value.value)
  , attribute(attributeRegistry[element->attribute.value])
{
  if ( attribute->isObject() ) {
    throw std::runtime_error("Content: attribute '" + attribute->id + "' is an object, which content cannot carry");
  }
}

