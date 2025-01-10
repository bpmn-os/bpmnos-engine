#include "BestFirstExit.h"
#include "execution/engine/src/Engine.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstExit::BestFirstExit(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
{
}

void BestFirstExit::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::ExitRequest
  );
  GreedyDispatcher::connect(mediator);
}

void BestFirstExit::notice(const Observable* observable) {
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


