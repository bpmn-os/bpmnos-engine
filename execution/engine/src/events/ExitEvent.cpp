#include "ExitEvent.h"

using namespace BPMNOS::Execution;

ExitEvent::ExitEvent(Token* token, std::optional<Values> exitStatus)
  : Event(token)
  , exitStatus(exitStatus)
{
}

