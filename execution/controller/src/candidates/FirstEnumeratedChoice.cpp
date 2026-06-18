#include "FirstEnumeratedChoice.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include <cassert>
#include <limits>

using namespace BPMNOS::Execution;

FirstEnumeratedChoice::FirstEnumeratedChoice(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

void FirstEnumeratedChoice::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Observable::Type::SystemState);
}

void FirstEnumeratedChoice::evaluateCandidates() {
  // stateless: recompute from scratch; determineBestChoices populates `candidates` with the considered
  // request's full alternative set, so stop at the first request that yields a feasible choice
  this->clearDecisions();
  for ( [[maybe_unused]] auto& [ pendingToken, pendingRequest ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = pendingRequest.lock() ) {
      if ( determineBestChoices( request ) ) {
        return;
      }
    }
  }
}

std::shared_ptr<Decision> FirstEnumeratedChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  this->clearDecisions();

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
    auto reward = decision->reward();
    if ( reward.has_value() && ( !bestDecision || reward.value() > bestDecision->reward().value() ) ) {
      bestDecision = decision;
    }
    // add every alternative to the reward-ordered candidates (infeasible last), taking ownership
    this->addCandidate( token_ptr, request_ptr, std::move(decision) );
  }

  return bestDecision;
}
