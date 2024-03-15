#ifndef BPMNOS_Execution_EventDispatcher_H
#define BPMNOS_Execution_EventDispatcher_H

#include "Decision.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Mediator;

class EventDispatcher {
public:
  virtual std::shared_ptr<Event> dispatchEvent( [[maybe_unused]] const SystemState* systemState ) = 0;
  void connect(Mediator* mediator);
  virtual void notice([[maybe_unused]] Decision* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EventDispatcher_H
