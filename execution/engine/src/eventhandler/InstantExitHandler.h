#ifndef BPMNOS_Execution_InstantExitHandler_H
#define BPMNOS_Execution_InstantExitHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an exit event for a token awaiting the exit at an activity that (is not within a sequential adhoc subprocess).
 */
class InstantExitHandler : public EventHandler {
public:
  InstantExitHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantExitHandler_H

