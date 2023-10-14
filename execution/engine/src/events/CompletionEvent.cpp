#include "CompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

CompletionEvent::CompletionEvent(const Token* token)
  : Event(token)
{
}

void CompletionEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
