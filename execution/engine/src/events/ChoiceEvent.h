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
  ChoiceEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& choices);
  void processBy(Engine* engine) const;
  std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > choices;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceEvent_H
