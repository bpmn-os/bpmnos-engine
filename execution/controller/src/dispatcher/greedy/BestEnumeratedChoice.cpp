#include "BestEnumeratedChoice.h"
#include "execution/engine/src/Mediator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

BestEnumeratedChoice::BestEnumeratedChoice(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
{
}

void BestEnumeratedChoice::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::ChoiceRequest
  );
  GreedyDispatcher::connect(mediator);
}


void BestEnumeratedChoice::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::ChoiceRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    // create pseudo decision
    auto decision = std::make_shared<ChoiceDecision>(request->token, std::vector<number>{}, evaluator);
    decisionStore.addDecision( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

std::shared_ptr<Event> BestEnumeratedChoice::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "BestEnumeratedChoice::dispatchEvent" << std::endl;
  decisionStore.advanceTime(systemState->currentTime);

  decisionStore.evaluateDecisions(
    [this]( std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> ) -> std::shared_ptr<Event> {
      auto request = request_ptr.lock();
      assert( request );
      // forget previous decision and find new best decision for the request
      auto decision = determineBestChoices( request );
      if ( decision ) {
        decisionStore.addEvaluation( token_ptr, request_ptr, std::move(decision) );
      }
      return nullptr;
    }
  );

  return decisionStore.getBestDecision();
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

