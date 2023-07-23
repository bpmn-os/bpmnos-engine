#ifndef BPMNOS_Decision_H
#define BPMNOS_Decision_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "model/parser/src/xml/bpmnos/tDecision.h"
#include "Attribute.h"

namespace BPMNOS {

class Decision {
public:
  Decision(XML::bpmnos::tDecision* decision, AttributeMap& attributeMap);
  XML::bpmnos::tDecision* element;

  Attribute* attribute;
};

} // namespace BPMNOS

#endif // BPMNOS_Decision_H
