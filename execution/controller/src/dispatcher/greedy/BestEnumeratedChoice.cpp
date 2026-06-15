#include "BestEnumeratedChoice.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

BestEnumeratedChoice::BestEnumeratedChoice(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

std::shared_ptr<Event> BestEnumeratedChoice::dispatchEvent( const SystemState* systemState ) {
  // stateless: dispatch the best feasible choice over the pending choice requests
  std::shared_ptr<Decision> best = nullptr;
  for ( auto& [ token_ptr, request_ptr ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = request_ptr.lock() ) {
      auto decision = determineBestChoices( request );
      if ( decision && ( !best || decision->reward().value() > best->reward().value() ) ) {
        best = std::move(decision);
      }
    }
  }
  return best;
}

std::shared_ptr<Decision> BestEnumeratedChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  auto token = request->token;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  auto decisionTask = token->node->as<BPMNOS::Model::DecisionTask>();
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  assert( token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->choices.size() );

  auto alternativeChoices = decisionTask->enumerateAlternatives(token->status, *token->data, token->globals);
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( auto& choices : alternativeChoices ) {
    auto decision = std::make_shared<ChoiceDecision>(token, std::move(choices), evaluator);
    decision->evaluate();
    if (
      decision->reward().has_value() &&
      ( !bestDecision || decision->reward().value() > bestDecision->reward().value() )
    ) {
      bestDecision = decision;
    }
  }
 
  // determine best alternative
  return bestDecision;
}

