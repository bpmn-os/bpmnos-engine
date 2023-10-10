#include "ReadyEvent.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(Token* token, std::optional<Values> readyStatus)
  : Event(token)
  , readyStatus(readyStatus)
{
}

