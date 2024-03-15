#ifndef BPMNOS_Execution_EventListener_H
#define BPMNOS_Execution_EventListener_H

#include "Event.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class EventDispatcher;

class EventListener {
public:
  void subscribe(EventDispatcher* eventDispatcher);
protected:
  std::vector<EventDispatcher*> eventDispatchers;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EventListener_H
