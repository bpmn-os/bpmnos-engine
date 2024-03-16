#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values updatedStatus)
  : Decision(token)
  , updatedStatus(updatedStatus)
{
}

void ChoiceDecision::processBy(Engine* engine) const {
  engine->process(this);
}
