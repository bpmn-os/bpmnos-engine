#ifndef BPMNOS_TerminationEvent_H
#define BPMNOS_TerminationEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an event causing the engine to terminate.
 */
struct TerminationEvent : Event {
  TerminationEvent();
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_TerminationEvent_H

