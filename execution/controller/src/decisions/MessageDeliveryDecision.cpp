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
  assert( event->token->busy() );
  assert( dynamic_cast<MessageDeliveryEvent*>(event) );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  double cost = (double)extensionElements->getObjective(status);

  auto message = dynamic_cast<MessageDeliveryEvent*>(event)->message;
  message->apply(event->token->node,status);
  
  extensionElements->applyOperators(status);
  cost -= (double)extensionElements->getObjective(status);
  return cost;
}

std::optional<double> MessageDeliveryDecision::guidedEvaluator(Event* event) {
  assert( event->token->ready() );
  // TODO
  return localEvaluator(event);
}


