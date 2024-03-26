#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token, BPMNOS::Values statusAttributes, BPMNOS::Values dataAttributes)
  : Event(token)
  , statusAttributes(statusAttributes)
  , dataAttributes(dataAttributes)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(this);
}
