#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values updatedStatus)
  : Event(token)
  , ChoiceEvent(token, updatedStatus)
  , Decision()
{
}

std::optional<double> ChoiceDecision::evaluate(Evaluator* evaluator) {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}
