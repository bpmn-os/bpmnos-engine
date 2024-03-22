#include "EntryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryDecision::EntryDecision(const Token* token, std::function<std::optional<double>(Event* event)> evaluator)
  : Event(token)
  , EntryEvent(token)
  , Decision(evaluator)
{
  evaluate();
}

std::optional<double> EntryDecision::evaluate() {
  evaluation = evaluator((EntryEvent*)this);
  return evaluation;
}

std::optional<double> EntryDecision::localEvaluator(Event* event) {
  assert( event->token->ready() );
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  double cost = (double)extensionElements->getObjective(status);
  extensionElements->applyOperators(status);
  cost -= (double)extensionElements->getObjective(status);
  return cost;
}

std::optional<double> EntryDecision::guidedEvaluator(Event* event) {
  assert( event->token->ready() );

  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( extensionElements->entryGuidance ) {
    auto systemState = event->token->owner->systemState;
    Values guidingAttributes; //TODO
    if ( systemState->assumedTime ) {
        guidingAttributes = systemState->scenario->getAnticipatedValues(event->token->node, event->token->status, systemState->currentTime );
    }
    else {
      auto values = systemState->scenario->getKnownValues(event->token->node, event->token->status, systemState->currentTime );
      if ( values ) {
        guidingAttributes = std::move( values.value() );
      }
    }
    
    auto guidance = extensionElements->entryGuidance->get()->apply(event->token->status, guidingAttributes);
    if ( guidance.has_value() ) {
      return (double)guidance.value();
    }
    else {
      return std::nullopt;
    }
  }

  return localEvaluator(event);
}


