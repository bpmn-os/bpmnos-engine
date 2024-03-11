#ifndef BPMNOS_Execution_ExitEvent_H
#define BPMNOS_Execution_ExitEvent_H

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
  constexpr Type getObservableType() const override { return Type::ExitEvent; };
  ExitEvent(const Token* token, std::optional<Values> exitStatus = std::nullopt);
  void processBy(Engine* engine) const override;
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ExitEvent_H

