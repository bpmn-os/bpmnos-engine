#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(Token* token, std::optional<Values> exitStatus)
  : Event(token)
  , exitStatus(exitStatus)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
