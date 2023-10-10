#include "ChoiceEvent.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(Token* token, std::optional<Values> exitStatus)
  : Event(token)
  , exitStatus(exitStatus)
{
}

