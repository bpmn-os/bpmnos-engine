#include "ClockTickEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ClockTickEvent::ClockTickEvent()
  : Event(nullptr)
{
}

void ClockTickEvent::processBy(Engine* engine) const {
  engine->process(this);
}
