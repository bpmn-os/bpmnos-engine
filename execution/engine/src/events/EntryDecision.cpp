#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, std::function<std::optional<double>(Decision* decision)> evaluator)
  : EntryEvent(token), Decision(evaluator)
{
  evaluate();
}

std::optional<double> EntryDecision::localEvaluator(Decision* decision) {
  assert( decision->token->ready() );
  auto extensionElements = decision->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = decision->token->status;
  double cost = (double)extensionElements->getObjective(status);
  extensionElements->applyOperators(status);
  cost -= (double)extensionElements->getObjective(status);
  return cost;
}

std::optional<double> EntryDecision::guidedEvaluator(Decision* decision) {
  assert( decision->token->ready() );
  // TODO
  return localEvaluator(decision);
}


