#include "ExitEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitEvent::ExitEvent(const Token* token, std::optional<Values> exitStatus)
  : Event(token)
  , exitStatus(exitStatus)
{
}

void ExitEvent::processBy(Engine* engine) const {
  engine->process(this);
}
