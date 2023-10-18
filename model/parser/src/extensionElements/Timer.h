#ifndef BPMNOS_Model_Timer_H
#define BPMNOS_Model_Timer_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class Timer : public BPMN::ExtensionElements {
public:
  Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::unique_ptr<Parameter> trigger;

  BPMNOS::number earliest(const BPMNOS::Values& values) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Timer_H
