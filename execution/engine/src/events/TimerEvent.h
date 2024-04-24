#ifndef BPMNOS_Execution_TimerEvent_H
#define BPMNOS_Execution_TimerEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event that a timer is triggered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct TimerEvent : virtual Event {
  TimerEvent(const Token* token);
  void processBy(Engine* engine) const;

  nlohmann::ordered_json jsonify() const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TimerEvent_H
