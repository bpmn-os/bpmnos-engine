#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token, std::function<std::optional<double>(Event* event)> evaluator)
  : Event(token)
  , ExitEvent(token)
  , Decision(evaluator)
{
  evaluate();
}

std::optional<double> ExitDecision::evaluate() {
  evaluation = evaluator((ExitEvent*)this);
  return evaluation;
}

std::optional<double> ExitDecision::localEvaluator([[maybe_unused]]Event* event) {
  return 0;
}

std::optional<double> ExitDecision::guidedEvaluator(Event* event) {
  assert( event->token->ready() );
  // TODO
  return localEvaluator(event);
}


