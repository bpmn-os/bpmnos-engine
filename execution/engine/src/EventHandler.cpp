#include "EventHandler.h"
#include "Engine.h"

using namespace BPMNOS::Execution;

void EventHandler::subscribe(Engine* engine) {
  engine->addEventHandler(this);
}

void EventHandler::notice([[maybe_unused]] Event* event) {
//std::cerr << "Notice pending event for token at node " << event->token->node->id << std::endl;
};

