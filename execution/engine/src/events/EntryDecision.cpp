#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, std::function<std::optional<double>(Decision* decision)> evaluator)
  : Decision(token, evaluator)
{
  evaluate();
}

EntryDecision::EntryDecision(const Token* token, Values entryStatus)
  : Decision(token)
  , entryStatus(entryStatus)
{
}

void EntryDecision::processBy(Engine* engine) const {
  engine->process(this);
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


