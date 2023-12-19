#include "TaskCompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TaskCompletionEvent::TaskCompletionEvent(const Token* token,  const Values& updatedStatus)
  : CompletionEvent(token)
  , updatedStatus(updatedStatus)
{
}

void TaskCompletionEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
