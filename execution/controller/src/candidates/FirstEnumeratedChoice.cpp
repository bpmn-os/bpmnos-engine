#include "FirstEnumeratedChoice.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstEnumeratedChoice::FirstEnumeratedChoice(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

void FirstEnumeratedChoice::evaluateCandidates(const SystemState* systemState) {
  // stateless: recompute from scratch, considering the pending choice requests in turn and stopping at
  // the first feasible one
  candidates.clear();
  bestChoice.reset();
  for ( auto& [ token_ptr, request_ptr ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = request_ptr.lock() ) {
      if ( auto decision = determineBestChoices( request ) ) {
        // own the decision so its weak_ptr in `candidates` stays valid for the dispatcher
        bestChoice = decision;
        candidates.emplace( -(double)decision->reward().value(), token_ptr, request_ptr, decision->weak_from_this(), decision->evaluation );
        return;
      }
    }
  }
}

std::shared_ptr<Decision> FirstEnumeratedChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
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
