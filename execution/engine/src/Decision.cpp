#include "Decision.h"

using namespace BPMNOS::Execution;


Decision::Decision(const Token* token, std::function<std::optional<double>(Decision* decision)> evaluator)
  : Event(token)
  , evaluator(evaluator)
{
}

std::optional<double> Decision::evaluate() {
  evaluation = evaluator(this);
  return evaluation;
}

