#include "TaskCompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TaskCompletionEvent::TaskCompletionEvent(const Token* token,  const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& updatedValues)
  : CompletionEvent(token)
  , updatedValues(updatedValues)
{
}

void TaskCompletionEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
