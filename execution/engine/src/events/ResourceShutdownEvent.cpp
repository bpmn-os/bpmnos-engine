#include "ResourceShutdownEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ResourceShutdownEvent::ResourceShutdownEvent(const Token* token)
  : CompletionEvent(token)
{
}

void ResourceShutdownEvent::processBy(Engine* engine) const {
//  token->systemState.tokensAwaitingChoiceEvent.
  engine->process(*this);
}
