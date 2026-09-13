#ifndef BPMNOS_Model_SignalDefinition_H
#define BPMNOS_Model_SignalDefinition_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

/**
 * @brief Class holding extension elements representing the definition of signal events
 *
 * The counterpart of @ref MessageDefinition for signals. It is what a node declares; the signal a run
 * broadcasts is @ref BPMNOS::Execution::Signal.
 **/
class SignalDefinition : public BPMN::ExtensionElements {
public:
  SignalDefinition(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  const AttributeRegistry& attributeRegistry;
  XML::bpmnos::tSignal* signal; ///< The `<bpmnos:signal>` element the signal is declared by, or nullptr where the node declares none.
  BPMNOS::number name; ///< Signal name
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
  struct { std::vector<const Attribute*> attributes; bool global = false; } dataUpdate; ///< Struct containing data attributes that are modified through signal content and a flag indicating whether a global value is changed (for catching signal events)
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_SignalDefinition_H
