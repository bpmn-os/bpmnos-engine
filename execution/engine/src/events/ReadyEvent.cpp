#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token, BPMNOS::Values values, BPMNOS::Values data)
  : Event(token)
  , values(values)
  , data(data)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(this);
}
