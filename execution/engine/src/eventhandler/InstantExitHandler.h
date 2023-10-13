#ifndef BPMNOS_InstantExitHandler_H
#define BPMNOS_InstantExitHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an exit event for a token awaiting the exit at an activity.
 */
class InstantExitHandler : public EventHandler {
  InstantExitHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_InstantExitHandler_H

