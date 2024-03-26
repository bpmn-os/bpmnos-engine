#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, const Message* message, std::function<std::optional<double>(const Event* event)> evaluator)
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

std::optional<double> MessageDeliveryDecision::localEvaluator(const Event* event) {
  assert( event->token->busy() );
  assert( dynamic_cast<const MessageDeliveryEvent*>(event) );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  double evaluation = (double)extensionElements->getObjective(status);

  auto message = dynamic_cast<const MessageDeliveryEvent*>(event)->message;
  message->apply(event->token->node,status);
  
  extensionElements->applyOperators(status);
  return evaluation - extensionElements->getObjective(status);
}

std::optional<double> MessageDeliveryDecision::guidedEvaluator(const Event* event) {
  assert( event->token->busy() );
  assert( dynamic_cast<const MessageDeliveryEvent*>(event) );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  auto evaluation = (double)extensionElements->getObjective(status);

  auto message = dynamic_cast<const MessageDeliveryEvent*>(event)->message;
  message->apply(event->token->node,status);
  
  extensionElements->applyOperators(status);

  if ( !extensionElements->messageDeliveryGuidance ) {
    return evaluation - extensionElements->getObjective(status);
  }

  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->messageDeliveryGuidance->get()->apply(systemState->scenario, systemState->currentTime, event->token->owner->root->instanceId, event->token->node, status);
  if ( guidance.has_value() ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


