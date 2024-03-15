#ifndef BPMNOS_Execution_EventDispatcher_H
#define BPMNOS_Execution_EventDispatcher_H

#include "Event.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Engine;

class EventDispatcher {
public:
  virtual std::shared_ptr<Event> dispatchEvent( [[maybe_unused]] const SystemState* systemState ) = 0;
  void subscribe(Engine* engine);
  virtual void notice([[maybe_unused]] Event* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EventDispatcher_H
