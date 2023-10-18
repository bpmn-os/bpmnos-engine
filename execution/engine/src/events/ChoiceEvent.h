#ifndef BPMNOS_Execution_ChoiceEvent_H
#define BPMNOS_Execution_ChoiceEvent_H

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
  ChoiceEvent(const Token* token, std::optional<Values> exitStatus);
  void processBy(Engine* engine) const override;
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceEvent_H

