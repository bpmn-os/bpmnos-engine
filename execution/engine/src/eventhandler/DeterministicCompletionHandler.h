#ifndef BPMNOS_DeterministicCompletionHandler_H
#define BPMNOS_DeterministicCompletionHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a completion event for a token awaiting the completion at an activity.
 */
struct DeterministicCompletionHandler : EventHandler {
  DeterministicCompletionHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_DeterministicCompletionHandler_H

