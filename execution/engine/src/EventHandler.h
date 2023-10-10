#ifndef BPMNOS_EventHandler_H
#define BPMNOS_EventHandler_H

#include "Event.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class EventHandler {
public:
  EventHandler();

  virtual std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_EventHandler_H
