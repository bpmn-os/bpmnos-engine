#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, std::optional<Values> entryStatus)
  : Decision(token)
  , entryStatus(entryStatus)
{
}

void EntryDecision::processBy(Engine* engine) const {
  engine->process(this);
}
