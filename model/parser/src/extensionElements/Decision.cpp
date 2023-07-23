#include "Decision.h"

using namespace BPMNOS;

Decision::Decision(XML::bpmnos::tDecision* decision, AttributeMap& attributeMap)
  : element(decision)
  , attribute(attributeMap.at(element->attribute.value))
{
}
