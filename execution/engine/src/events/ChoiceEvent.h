#ifndef BPMNOS_Execution_ChoiceEvent_H
#define BPMNOS_Execution_ChoiceEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/CompletionEvent.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of choices that are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceEvent : CompletionEvent {
  ChoiceEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& choices);
  void processBy(Engine* engine) const override;
  std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceEvent_H

