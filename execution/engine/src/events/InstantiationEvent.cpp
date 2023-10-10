#include "InstantiationEvent.h"

using namespace BPMNOS::Execution;

InstantiationEvent::InstantiationEvent(const BPMN::Process* process, Values status)
  : Event(nullptr)
  , process(process)
  , status(status)
{
}

