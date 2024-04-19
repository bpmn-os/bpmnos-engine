#ifndef BPMNOS_Model_Timer_H
#define BPMNOS_Model_Timer_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

/**
 * @brief Class holding extension elements representing the trigger of timer events 
 **/
class Timer : public BPMN::ExtensionElements {
public:
  Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  const AttributeRegistry& attributeRegistry;
  std::unique_ptr<BPMNOS::Model::Parameter> trigger;

  template <typename DataType>
  BPMNOS::number earliest(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Timer_H
