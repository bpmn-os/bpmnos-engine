#include "BestFirstEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstEntry::BestFirstEntry(Evaluator* evaluator, Config config)
  : GreedyDispatcher(evaluator)
  , config(config)
{
}

void BestFirstEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest
  );
  GreedyDispatcher::connect(mediator);
}

void BestFirstEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    if ( config.onlySequential ) {
      assert( request->token->node->parent );
      if ( !request->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        // only entries of sequential ad-hoc subprocess children are handled
        return;
      }
    }
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}


