#ifndef BPMNOS_ClockTickEvent_H
#define BPMNOS_ClockTickEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an event that increments the current time.
 *
 * This event contains a pointer to the process model and a unique instance identifier.
 */
struct ClockTickEvent : Event {
  ClockTickEvent();
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ClockTickEvent_H

