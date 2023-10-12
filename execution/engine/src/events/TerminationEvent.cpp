#include "TerminationEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TerminationEvent::TerminationEvent()
  : Event(nullptr)
{
}

void TerminationEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
