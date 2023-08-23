#include "Decision.h"

using namespace BPMNOS::Model;

Decision::Decision(XML::bpmnos::tDecision* decision, AttributeMap& attributeMap)
  : element(decision)
  , attribute(attributeMap.at(element->attribute.value))
{
  attribute->isImmutable = false;
}
