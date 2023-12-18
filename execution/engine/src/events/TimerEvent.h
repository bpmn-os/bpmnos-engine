#ifndef BPMNOS_Execution_TimerEvent_H
#define BPMNOS_Execution_TimerEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a timer being triggered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct TimerEvent: Event {
  TimerEvent(const Token* token);
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TimerEvent_H
