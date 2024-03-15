#include "EventDispatcher.h"
#include "Engine.h"

using namespace BPMNOS::Execution;

void EventDispatcher::subscribe(Engine* engine) {
  engine->addEventDispatcher(this);
}

void EventDispatcher::notice([[maybe_unused]] Event* event) {
//std::cerr << "Notice pending event for token at node " << event->token->node->id << std::endl;
};

