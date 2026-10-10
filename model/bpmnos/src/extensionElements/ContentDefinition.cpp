#include "ContentDefinition.h"

using namespace BPMNOS::Model;

ContentDefinition::ContentDefinition(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry)
  : element(content)
  , key(content->key.value.value)
  , attribute(attributeRegistry[element->attribute.value])
{
  if ( attribute->isObject() ) {
    throw std::runtime_error("ContentDefinition: attribute '" + attribute->id + "' is an object, which content cannot carry");
  }
}

