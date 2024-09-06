#ifndef BPMNOS_Model_SignalDefinition_H
#define BPMNOS_Model_SignalDefinition_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"

namespace BPMNOS::Model {

class SignalDefinition {
public:
  SignalDefinition(XML::bpmnos::tSignal* signal, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tSignal* element;
  BPMNOS::number name; ///< Signal name
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SignalDefinition_H
