#include "TimerEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TimerEvent::TimerEvent(const Token* token)
  : Event(token)
{
}

void TimerEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
