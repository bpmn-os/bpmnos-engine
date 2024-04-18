#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token, Evaluator* evaluator)
  : Event(token)
  , ExitEvent(token)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> ExitDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}
