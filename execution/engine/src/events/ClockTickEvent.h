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
  /// Schedules the tick one clockTick ahead of the current time.
  ClockTickEvent(const SystemState* systemState);

  /**
   * @brief Schedules the tick at the given time.
   *
   * Only permitted before a run has started, i.e. while the system state stands at the lowest
   * representable time. During a run the clock must visit every instant, because instances are
   * instantiated at the instant their instantiation time is reached.
   *
   * @param systemState The system state the tick refers to
   * @param time Time the tick advances to
   */
  ClockTickEvent(const SystemState* systemState, BPMNOS::number time);

  void processBy(Engine* engine) const override;

  /// Stale once live time already reached the scheduled tick time (time must strictly advance).
  bool expired() const override;

  nlohmann::ordered_json jsonify() const override;

  static constexpr BPMNOS::number clockTick = 1; ///< Timestep used to advance the current time
  BPMNOS::number time;
  const SystemState* systemState;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ClockTickEvent_H

