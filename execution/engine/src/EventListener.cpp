#include "EventListener.h"
#include "Engine.h"

using namespace BPMNOS::Execution;

void EventListener::subscribe(EventDispatcher* eventDispatcher) {
  eventDispatchers.push_back(eventDispatcher);
}

