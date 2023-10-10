#include "EntryEvent.h"

using namespace BPMNOS::Execution;

EntryEvent::EntryEvent(Token* token, std::optional<Values> entryStatus)
  : Event(token)
  , entryStatus(entryStatus)
{
}

