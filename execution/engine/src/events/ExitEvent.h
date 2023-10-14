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
  ExitEvent(const Token* token, std::optional<Values> exitStatus = std::nullopt);
  void processBy(Engine* engine) const override;
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ExitEvent_H

