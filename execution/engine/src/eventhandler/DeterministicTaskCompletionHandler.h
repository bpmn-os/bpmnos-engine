#ifndef BPMNOS_Execution_DeterministicTaskCompletionHandler_H
#define BPMNOS_Execution_DeterministicTaskCompletionHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a completion event for a token awaiting the completion of a task (except DecisionTask).
 */
class DeterministicTaskCompletionHandler : public EventHandler {
public:
  DeterministicTaskCompletionHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DeterministicTaskCompletionHandler_H

