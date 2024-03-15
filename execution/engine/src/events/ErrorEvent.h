#ifndef BPMNOS_Execution_ErrorEvent_H
#define BPMNOS_Execution_ErrorEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of an error being raised.
 *
 * Transition from any state to State::FAILED
 */
struct ErrorEvent : Event {
  ErrorEvent(const Token* token);
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ErrorEvent_H

