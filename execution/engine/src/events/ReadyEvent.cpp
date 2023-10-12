#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(Token* token, std::optional<Values> readyStatus)
  : Event(token)
  , readyStatus(readyStatus)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
