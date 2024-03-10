#include "EventHandler.h"
#include "Engine.h"

using namespace BPMNOS::Execution;

void EventHandler::subscribe(Engine* engine) {
  engine->addEventHandler(this);
}

