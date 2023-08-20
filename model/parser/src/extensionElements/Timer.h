#ifndef BPMNOS_Timer_H
#define BPMNOS_Timer_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "model/utility/src/Number.h"

namespace BPMNOS {

class Timer : public BPMN::ExtensionElements {
public:
  Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::unique_ptr<Parameter> trigger;
  Attribute* timestampAttribute;

  number earliest(const Values& values) const;
};

} // namespace BPMNOS

#endif // BPMNOS_Timer_H
