#ifndef BPMNOS_InstantiationEvent_H
#define BPMNOS_InstantiationEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an event that occurs when a new BPMN instance is being instantiated.
 *
 * This event contains a pointer to the process model and an initial status including the
 * instance id and other known attribute values
 */
struct InstantiationEvent : Event {
  InstantiationEvent(const BPMN::Process* process, Values status);
  void processBy(Engine* engine) const override;
  const BPMN::Process* process;
  Values status; ///< Initial status including instance id and other known attribute values.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_InstantiationEvent_H

