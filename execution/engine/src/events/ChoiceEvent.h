#ifndef BPMNOS_Execution_ChoiceEvent_H
#define BPMNOS_Execution_ChoiceEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event that choices are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceEvent : Event {
  constexpr Type getObservableType() const override { return Type::ChoiceEvent; };
  ChoiceEvent(const Token* token);
  void processBy(Engine* engine) const;
  Values updatedStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceEvent_H
