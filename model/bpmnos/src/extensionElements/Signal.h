#ifndef BPMNOS_Model_Signal_H
#define BPMNOS_Model_Signal_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

/**
 * @brief Class holding extension elements representing the definition of signal events 
 **/
class Signal : public BPMN::ExtensionElements {
public:
  Signal(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  const AttributeRegistry& attributeRegistry;
  BPMNOS::number name; ///< Signal name
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
  std::vector<const Attribute*> updatedData; ///< Vector containing data attributes that are modified (for catching signal events)
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Signal_H
