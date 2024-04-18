#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, Evaluator* evaluator)
  : Event(token)
  , EntryEvent(token)
  , Decision(evaluator)
{
  determineDependencies( evaluator->getDependencies(this) );
}

std::optional<double> EntryDecision::evaluate() {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}
