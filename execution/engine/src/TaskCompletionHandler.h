#ifndef BPMNOS_Execution_TaskCompletionHandler_H
#define BPMNOS_Execution_TaskCompletionHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief EventDispatcher for task completion events.
 *
 * Iterates tokensAwaitingCompletionEvent and dispatches CompletionEvent
 * with status from scenario->getTaskCompletionStatus().
 */
class TaskCompletionHandler : public EventDispatcher {
public:
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TaskCompletionHandler_H
