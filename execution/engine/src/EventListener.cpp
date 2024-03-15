#include "EventListener.h"
#include "Engine.h"
#include "Event.h"

using namespace BPMNOS::Execution;

void EventListener::subscribe(EventDispatcher* eventDispatcher) {
  eventDispatchers.push_back(eventDispatcher);
}

std::shared_ptr<Event> EventListener::fetchEvent(SystemState* systemState) {
  for ( auto eventDispatcher : eventDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      return event;
    }
  }
  return nullptr;
}

