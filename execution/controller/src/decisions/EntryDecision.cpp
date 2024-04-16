#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token)
  : Event(token)
  , EntryEvent(token)
  , Decision()
{
}

std::optional<double> EntryDecision::evaluate(Evaluator* evaluator) {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}
