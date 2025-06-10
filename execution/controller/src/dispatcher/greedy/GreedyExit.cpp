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
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}


std::shared_ptr<Event> GreedyExit::dispatchEvent( const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for (auto it = decisionsWithoutEvaluation.begin(); it != decisionsWithoutEvaluation.end(); ) {
    auto [ token_ptr, request_ptr, decision ] = std::move(*it);  // Move out the tuple to avoid dangling reference
    it = decisionsWithoutEvaluation.erase(it);
    assert(decision);

    // Call decision->evaluate() and add the evaluation
    auto reward = decision->evaluate();
    addEvaluation(token_ptr, request_ptr, decision, reward);
    if (  reward.has_value() ) {
      return std::make_shared<ExitEvent>(decision->token);
    }
  }
  
  // all evaluated decisions are infeasible unless a previously dispatched decision was not deployed
  for ( auto decisionTuple : evaluatedDecisions ) {
    constexpr std::size_t last = std::tuple_size<decltype(decisionTuple)>::value - 1;
    std::weak_ptr<Event>& event_ptr = std::get<last>(decisionTuple);
    if ( auto event = event_ptr.lock();
      event && std::get<0>(decisionTuple) < std::numeric_limits<double>::max()
    ) {
//std::cerr << "\nBest decision " << event->jsonify() << " evaluated with " << std::get<0>(decisionTuple) << std::endl;
      return event;
    }
    else {
      // best decision is infeasible, no need to inspect others
      break;
    }
  }

  return nullptr;
}

