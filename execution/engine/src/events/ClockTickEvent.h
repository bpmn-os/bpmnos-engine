#ifndef BPMNOS_Execution_ClockTickEvent_H
#define BPMNOS_Execution_ClockTickEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an event that increments the current time.
 *
 * This event contains a pointer to the process model and a unique instance identifier.
 */
struct ClockTickEvent : Event {
  constexpr Type getObservableType() const override { return Type::ClockTickEvent; };
  ClockTickEvent();
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ClockTickEvent_H

