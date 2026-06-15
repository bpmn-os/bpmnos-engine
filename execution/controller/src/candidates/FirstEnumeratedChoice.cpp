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
  // stateless: recompute from scratch; determineBestChoices populates `candidates` with the considered
  // request's full alternative set, so stop at the first request that yields a feasible choice
  candidates.clear();
  evaluatedChoices.clear();
  for ( [[maybe_unused]] auto& [ pendingToken, pendingRequest ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = pendingRequest.lock() ) {
      if ( determineBestChoices( request ) ) {
        return;
      }
    }
  }
}

std::shared_ptr<Decision> FirstEnumeratedChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  candidates.clear();
  evaluatedChoices.clear();

  auto token = request->token;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  auto decisionTask = token->node->as<BPMNOS::Model::DecisionTask>();
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  assert( token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->choices.size() );

  auto token_ptr = token->weak_from_this();
  auto request_ptr = request->weak_from_this();

  auto alternativeChoices = decisionTask->enumerateAlternatives(token->status, *token->data, token->globals);
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( auto& choices : alternativeChoices ) {
    auto decision = std::make_shared<ChoiceDecision>(token, std::move(choices), evaluator);
    decision->evaluate();
    if ( decision->reward().has_value() && ( !bestDecision || decision->reward().value() > bestDecision->reward().value() ) ) {
      bestDecision = decision;
    }
    // keep every evaluated alternative alive (for rollout); only the best is offered to the dispatcher
    evaluatedChoices.emplace_back( token_ptr, request_ptr, std::move(decision) );
  }

  // only the (first-encountered) best feasible choice is offered to the greedy dispatcher (deterministic on ties)
  if ( bestDecision ) {
    candidates.emplace( -(double)bestDecision->reward().value(), token_ptr, request_ptr, bestDecision->weak_from_this(), bestDecision->evaluation );
  }
  return bestDecision;
}
