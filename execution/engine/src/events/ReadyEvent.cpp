#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token)
  : Event(token)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(this);
}
