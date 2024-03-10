#ifndef BPMNOS_Execution_EventHandler_H
#define BPMNOS_Execution_EventHandler_H

#include "Event.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Engine;

class EventHandler {
public:
  virtual std::shared_ptr<Event> dispatchEvent( [[maybe_unused]] const SystemState* systemState ) = 0;
  void subscribe(Engine* engine);
  void notice([[maybe_unused]] Event* event) {};
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EventHandler_H
