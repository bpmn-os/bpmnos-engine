#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values updatedStatus, std::function<std::optional<double>(Event* event)> evaluator)
  : Event(token)
  , ChoiceEvent(token, updatedStatus)
  , Decision(evaluator)
{
  evaluate();
}

std::optional<double> ChoiceDecision::evaluate() {
  evaluation = evaluator((ChoiceEvent*)this);
  return evaluation;
}

std::optional<double> ChoiceDecision::localEvaluator(Event* event) {
  assert( event->token->ready() );
  assert( dynamic_cast<ChoiceEvent*>(event) );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  double cost = (double)extensionElements->getObjective(event->token->status);
  Values status = dynamic_cast<ChoiceEvent*>(event)->updatedStatus;
  extensionElements->applyOperators(status);
  cost -= (double)extensionElements->getObjective(status);
  return cost;
}

std::optional<double> ChoiceDecision::guidedEvaluator(Event* event) {
  assert( event->token->ready() );
  // TODO
  return localEvaluator(event);
}


