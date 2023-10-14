#include "EntryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryEvent::EntryEvent(const Token* token, std::optional<Values> entryStatus)
  : Event(token)
  , entryStatus(entryStatus)
{
}

void EntryEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
