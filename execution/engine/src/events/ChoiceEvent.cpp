#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(const Token* token, Values updatedStatus)
  : Event(token)
  , updatedStatus(updatedStatus)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
  engine->process(this);
}
