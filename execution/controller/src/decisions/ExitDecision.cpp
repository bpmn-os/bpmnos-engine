#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token, std::function<std::optional<double>(const Event* event)> evaluator)
  : Event(token)
  , ExitEvent(token)
  , Decision(evaluator)
{
  evaluate();
  if (*evaluator.target<std::optional<double> (*)(const BPMNOS::Execution::Event*)>() == &ExitDecision::guidedEvaluator) {
    auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( extensionElements->exitGuidance.has_value() ) {
      determineDependencies( extensionElements->exitGuidance.value()->dependencies );
    }
  }
}

std::optional<double> ExitDecision::evaluate() {
  evaluation = evaluator((ExitEvent*)this);
  return evaluation;
}

std::optional<double> ExitDecision::localEvaluator([[maybe_unused]]const Event* event) {
  assert( event->token->completed() );
  return 0;
}

std::optional<double> ExitDecision::guidedEvaluator(const Event* event) {
  assert( event->token->completed() );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( !extensionElements->exitGuidance ) {
    return 0;
  }

  Values status = event->token->status;
  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = event->token->owner->systemState->currentTime;
  Values data(*event->token->data);
  auto evaluation = (double)extensionElements->getObjective(status,data);
    
  auto systemState = event->token->owner->systemState;
  auto guidance = extensionElements->exitGuidance->get()->apply(systemState->scenario, systemState->currentTime, event->token->owner->root->instance.value(), event->token->node, status, data);
  if ( guidance.has_value() ) {
    return evaluation - guidance.value();
  }

  return std::nullopt;

}


