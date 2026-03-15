#ifndef BPMNOS_Execution_ClockTickEvent_H
#define BPMNOS_Execution_ClockTickEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

class SystemState;

/**
 * @brief Represents an event that increments the current time.
 *
 * This event contains a pointer to the system state for observers to access.
 */
struct ClockTickEvent : Event {
  ClockTickEvent(const SystemState* systemState);
  void processBy(Engine* engine) const override;

  nlohmann::ordered_json jsonify() const override;

  const SystemState* systemState;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ClockTickEvent_H

