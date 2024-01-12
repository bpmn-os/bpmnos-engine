#include "ErrorEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ErrorEvent::ErrorEvent(const Token* token)
  : Event(token)
{
}

void ErrorEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
