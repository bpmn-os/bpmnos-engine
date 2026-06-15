#include "GreedyExit.h"
#include "execution/engine/src/Engine.h"
#include <cassert>

using namespace BPMNOS::Execution;

GreedyExit::GreedyExit(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
{
}

void GreedyExit::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::ExitRequest
  );
  GreedyDispatcher::connect(mediator);
}

void GreedyExit::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::ExitRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    auto decision = std::make_shared<ExitDecision>(request->token, evaluator);
    decisionStore.addDecision( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}


std::shared_ptr<Event> GreedyExit::dispatchEvent( const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  decisionStore.advanceTime(systemState->currentTime);

  if ( auto event = decisionStore.evaluateDecisions(
    [this]( std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision ) -> std::shared_ptr<Event> {
      assert(decision);
      // Call decision->evaluate() and add the evaluation
      decision->evaluate();
      decisionStore.addEvaluation(token_ptr, request_ptr, decision);
      if ( decision->reward().has_value() ) {
        return std::make_shared<ExitEvent>(decision->token);
      }
      return nullptr;
    }
  ) ) {
    return event;
  }

  // all evaluated decisions are infeasible unless a previously dispatched decision was not deployed
  return decisionStore.getBestDecision();
}

