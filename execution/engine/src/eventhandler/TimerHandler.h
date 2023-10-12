#ifndef BPMNOS_TimerHandler_H
#define BPMNOS_TimerHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a timer event for a token awaiting the trigger at a catching timer event.
 */
class TimerHandler : public EventHandler {
  TimerHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_TimerHandler_H

