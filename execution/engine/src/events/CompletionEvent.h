#ifndef BPMNOS_Execution_CompletionEvent_H
#define BPMNOS_Execution_CompletionEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token having completed an activity.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct CompletionEvent : Event {
  CompletionEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& updatedValues);
  void processBy(Engine* engine) const override;
  std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CompletionEvent_H

