#include "InstantiationEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

InstantiationEvent::InstantiationEvent(const BPMN::Process* process, Values status)
  : Event(nullptr)
  , process(process)
  , status(status)
{
}

void InstantiationEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
