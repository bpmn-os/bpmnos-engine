#include "ChoiceDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceDecision::ChoiceDecision(const Token* token, Values updatedStatus, std::function<std::optional<double>(const Event* event)> evaluator)
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

std::optional<double> ChoiceDecision::localEvaluator(const Event* event) {
  assert( event->token->busy() );
  assert( dynamic_cast<const ChoiceEvent*>(event) );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto evaluation = (double)extensionElements->getObjective(event->token->status);
  Values status = dynamic_cast<const ChoiceEvent*>(event)->updatedStatus;
  extensionElements->applyOperators(status);
  return evaluation - extensionElements->getObjective(status);
}

std::optional<double> ChoiceDecision::guidedEvaluator(const Event* event) {
  assert( event->token->busy() );
  assert( dynamic_cast<const ChoiceEvent*>(event) );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto evaluation = (double)extensionElements->getObjective(event->token->status);
  Values status = dynamic_cast<const ChoiceEvent*>(event)->updatedStatus;
  extensionElements->applyOperators(status);

  if ( !extensionElements->choiceGuidance ) {
    return evaluation - extensionElements->getObjective(status);
  }

  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->choiceGuidance->get()->apply(status, event->token->node, systemState->scenario, systemState->currentTime);
  if ( guidance.has_value() ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;
}


