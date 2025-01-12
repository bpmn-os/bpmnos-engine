#include "Content.h"

using namespace BPMNOS::Model;

Content::Content(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry)
  : element(content)
  , id(content->id.value.value)
  , key(content->key.value.value)
  , attribute(attributeRegistry[element->attribute.value])
{
}

