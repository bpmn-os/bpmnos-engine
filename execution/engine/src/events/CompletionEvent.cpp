#include "CompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

CompletionEvent::CompletionEvent(const Token* token,  const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& updatedValues)
  : Event(token)
  , updatedValues(updatedValues)
{
}

void CompletionEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
