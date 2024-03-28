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
  Values data(*event->token->data);
  double evaluation = (double)extensionElements->getObjective(status,data);

  auto message = dynamic_cast<const MessageDeliveryEvent*>(event)->message;
  message->apply(event->token->node,event->token->getAttributeRegistry(),status,data);
  
  extensionElements->applyOperators(status,data);
  return evaluation - extensionElements->getObjective(status,data);
}

std::optional<double> MessageDeliveryDecision::guidedEvaluator(const Event* event) {
  assert( event->token->busy() );
  assert( dynamic_cast<const MessageDeliveryEvent*>(event) );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  Values data(*event->token->data);
  auto evaluation = (double)extensionElements->getObjective(status,data);

  auto message = dynamic_cast<const MessageDeliveryEvent*>(event)->message;
  message->apply(event->token->node,event->token->getAttributeRegistry(),status,data);
  
  extensionElements->applyOperators(status,data);

  if ( !extensionElements->messageDeliveryGuidance ) {
    return evaluation - extensionElements->getObjective(status,data);
  }

  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->messageDeliveryGuidance->get()->apply(systemState->scenario, systemState->currentTime, event->token->owner->root->instanceId, event->token->node, status, data);
  if ( guidance.has_value() ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


