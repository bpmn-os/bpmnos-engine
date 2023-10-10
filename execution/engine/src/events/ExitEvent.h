#ifndef BPMNOS_ExitEvent_H
#define BPMNOS_ExitEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token exiting a node.
 *
 * Transition from State::COMPLETION to State::DONE or State::DEPARTED
 */
struct ExitEvent : Event {
  ExitEvent(Token* token, std::optional<Values> exitStatus = std::nullopt);
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ExitEvent_H

