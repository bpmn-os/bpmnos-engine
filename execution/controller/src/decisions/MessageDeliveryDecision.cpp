#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, const Message* message, std::function<std::optional<double>(Event* event)> evaluator)
  : Event(token)
  , MessageDeliveryEvent(token,message)
  , Decision(evaluator)
{
  evaluate();
}

std::optional<double> MessageDeliveryDecision::evaluate() {
  evaluation = evaluator((MessageDeliveryEvent*)this);
  return evaluation;
}

std::optional<double> MessageDeliveryDecision::localEvaluator(Event* event) {
  assert( event->token->ready() );
  assert( dynamic_cast<MessageDeliveryEvent*>(event) );
/*
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  double cost = (double)extensionElements->getObjective(event->token->status);
  Values status = static_cast<MessageDeliveryEvent*>(event)->updatedStatus;
  extensionElements->applyOperators(status);
  cost -= (double)extensionElements->getObjective(status);
  return cost;
*/
  return 0;
}

std::optional<double> MessageDeliveryDecision::guidedEvaluator(Event* event) {
  assert( event->token->ready() );
  // TODO
  return localEvaluator(event);
}


