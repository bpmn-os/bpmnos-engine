#include "TriggerEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TriggerEvent::TriggerEvent(const Token* token)
  : Event(token)
{
}

void TriggerEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
