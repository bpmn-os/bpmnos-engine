#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& updatedValues)
  : CompletionEvent(token)
  , updatedValues(updatedValues)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
//  token->systemState.tokensAwaitingChoiceEvent.
  engine->process(*this);
}
