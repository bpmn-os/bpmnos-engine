#ifndef BPMNOS_Model_Decision_H
#define BPMNOS_Model_Decision_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "model/parser/src/xml/bpmnos/tDecision.h"
#include "Attribute.h"

namespace BPMNOS::Model {

class Decision {
public:
  Decision(XML::bpmnos::tDecision* decision, AttributeMap& attributeMap);
  XML::bpmnos::tDecision* element;

  Attribute* attribute;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Decision_H
