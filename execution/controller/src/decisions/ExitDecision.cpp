#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token)
  : Event(token)
  , ExitEvent(token)
  , Decision()
{
}

std::optional<double> ExitDecision::evaluate(Evaluator* evaluator) {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}
