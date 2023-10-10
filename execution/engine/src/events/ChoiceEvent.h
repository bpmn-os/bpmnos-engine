#ifndef BPMNOS_ChoiceEvent_H
#define BPMNOS_ChoiceEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of choices that are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceEvent : Event {
  ChoiceEvent(Token* token, std::optional<Values> exitStatus);
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ChoiceEvent_H

