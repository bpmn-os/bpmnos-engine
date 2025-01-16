#include "Content.h"

using namespace BPMNOS::Model;

Content::Content(XML::bpmnos::tContent* content, const AttributeRegistry& attributeRegistry)
  : element(content)
  , key(content->key.value.value)
  , attribute(attributeRegistry[element->attribute.value])
{
}

