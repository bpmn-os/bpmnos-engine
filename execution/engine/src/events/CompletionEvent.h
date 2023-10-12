#ifndef BPMNOS_CompletionEvent_H
#define BPMNOS_CompletionEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token having completed an activity.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct CompletionEvent : Event {
  CompletionEvent(Token* token);
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_CompletionEvent_H

