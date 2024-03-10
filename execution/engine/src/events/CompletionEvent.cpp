#include "CompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

CompletionEvent::CompletionEvent(const Token* token,  std::optional<Values> updatedStatus)
  : Event(token)
  , updatedStatus(updatedStatus)
{
}

void CompletionEvent::processBy(Engine* engine) const {
  engine->process(this);
}
