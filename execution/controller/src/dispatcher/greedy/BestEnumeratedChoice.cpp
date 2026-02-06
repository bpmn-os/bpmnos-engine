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
    auto decision = std::make_shared<ChoiceDecision>(request->token, Values(), evaluator);
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

std::shared_ptr<Event> BestEnumeratedChoice::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "BestEnumeratedChoice::dispatchEvent" << std::endl;
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for ( auto& [ token_ptr, request_ptr, _ ] : decisionsWithoutEvaluation ) {
    auto request = request_ptr.lock();
    assert( request );
    // forget previous decision and find new best decision for the request
    auto decision = determineBestChoices( request );
    if ( decision ) {
      addEvaluation( token_ptr, request_ptr, std::move(decision), decision->reward );
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, event_ptr ] : evaluatedDecisions ) {
//std::cerr << "Best choice decision " << event_ptr.lock()->jsonify() << " evaluated with " << cost << std::endl;
    return event_ptr.lock();
  }

//std::cerr << "No evaluated choice decision" << std::endl;
  return nullptr;
}

std::shared_ptr<Decision> BestEnumeratedChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  auto token = request->token;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  auto decisionTask = token->node->as<BPMNOS::Model::DecisionTask>();
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements->choices.size() );

  std::vector< BPMNOS::Values > alternativeChoices = decisionTask->enumerateAlternatives(token->status, *token->data, token->globals);
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( auto& choices : alternativeChoices ) {
    auto decision = std::make_shared<ChoiceDecision>(token,choices,evaluator);
    decision->evaluate();
    if (
      decision->reward.has_value() &&
      ( !bestDecision || decision->reward.value() > bestDecision->reward.value() )
    ) {
      bestDecision = decision;
    }
  }
 
  // determine best alternative
  return bestDecision;
}

