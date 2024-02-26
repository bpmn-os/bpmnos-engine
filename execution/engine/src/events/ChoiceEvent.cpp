#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& choices)
  : Event(token)
  , choices(choices)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
