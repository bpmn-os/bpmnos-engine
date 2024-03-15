#include "EventDispatcher.h"
#include "Mediator.h"

using namespace BPMNOS::Execution;

void EventDispatcher::connect(Mediator* mediator) {
  mediator->subscribe(this);
}

void EventDispatcher::notice([[maybe_unused]] Decision* event) {
//std::cerr << "Notice pending event for token at node " << event->token->node->id << std::endl;
};

