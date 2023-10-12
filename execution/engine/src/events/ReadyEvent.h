#ifndef BPMNOS_ReadyEvent_H
#define BPMNOS_ReadyEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token ready to enter a node inclduing necressary new attribute data.
 *
 * Transition from State::CREATED or State::ARRIVED to State::READY
 */
struct ReadyEvent : Event {
  ReadyEvent(Token* token, std::optional<Values> readyStatus = std::nullopt);
  void processBy(Engine* engine) const override;
  std::optional<Values> readyStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ReadyEvent_H

