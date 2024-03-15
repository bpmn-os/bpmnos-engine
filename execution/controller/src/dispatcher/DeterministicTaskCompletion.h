#ifndef BPMNOS_Execution_DeterministicTaskCompletion_H
#define BPMNOS_Execution_DeterministicTaskCompletion_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a completion event for a token awaiting the completion of a task (except DecisionTask).
 */
class DeterministicTaskCompletion : public EventDispatcher {
public:
  DeterministicTaskCompletion();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DeterministicTaskCompletion_H

