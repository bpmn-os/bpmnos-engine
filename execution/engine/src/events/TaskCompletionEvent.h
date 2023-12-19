#ifndef BPMNOS_Execution_TaskCompletionEvent_H
#define BPMNOS_Execution_TaskCompletionEvent_H

#include <bpmn++.h>
#include "CompletionEvent.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token having completed an activity.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct TaskCompletionEvent : CompletionEvent  {
  TaskCompletionEvent(const Token* token, const Values& updatedStatus);
  void processBy(Engine* engine) const override;
  Values updatedStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TaskCompletionEvent_H

