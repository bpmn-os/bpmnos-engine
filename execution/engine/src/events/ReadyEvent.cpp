#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token, BPMNOS::Values values)
  : Event(token)
  , values(values)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
