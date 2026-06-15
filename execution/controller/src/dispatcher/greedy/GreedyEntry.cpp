#include "GreedyEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

GreedyEntry::GreedyEntry(Evaluator* evaluator, Config config)
  : GreedyDispatcher(evaluator)
  , config(config)
{
}

void GreedyEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest
  );
  GreedyDispatcher::connect(mediator);
}

void GreedyEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    if ( !config.sequential ) {
      assert( request->token->node->parent );
      if ( request->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        // entries of sequential ad-hoc subprocess children are left to another dispatcher
        return;
      }
    }
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
    decisionStore.decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}


std::shared_ptr<Event> GreedyEntry::dispatchEvent( const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  if ( systemState->currentTime > decisionStore.timestamp ) {
    decisionStore.timestamp = systemState->currentTime;
    decisionStore.clockTick();
  }

  for (auto it = decisionStore.decisionsWithoutEvaluation.begin(); it != decisionStore.decisionsWithoutEvaluation.end(); ) {
    auto [ token_ptr, request_ptr, decision  ] = std::move(*it);
    it = decisionStore.decisionsWithoutEvaluation.erase(it);
    assert(decision);
    auto token = token_ptr.lock();
    assert( token );
    assert( token->node->parent );
    
    if ( !token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
      // evaluation of decision that is independent of others
      decision->evaluate();
//std::cerr << "Regular: " << decision->jsonify() << std::endl;
      decisionStore.addEvaluation(token_ptr, request_ptr, decision);

      if ( decision->reward().has_value() ) {
        // dispatch feasible decision 
        return std::make_shared<EntryEvent>(decision->token);
      }
    }
    else {
      // sequential ad-hoc subprocess child entry (only registered when config.sequential):
      // defer for performer grouping
      unevaluatedSequentialEntries.emplace_back(
        std::move(token_ptr), std::move(request_ptr), std::move(decision)
      );
    }
  }

  // all evaluated decisions are infeasible unless a previously dispatched decision was not deployed
  // Warning: event is dispatched even if it is no longer the best due to data updates since undeployed dispatch
  if ( auto event = decisionStore.getBestDecision() ) {
    return event;
  }

  std::unordered_map<const Token*, auto_list<std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > > sequentialActivityEntries;

  for (auto& [ token_ptr, request_ptr, decision  ] : unevaluatedSequentialEntries ) {
    assert(decision);
    auto token = token_ptr.lock();
    assert( token );
    assert( token->node->parent );
    assert( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );
    // defer entry decision for sequential activities
    auto tokenAtSequentialPerformer = token->getSequentialPerformerToken();
//std::cerr << "Defer: " << token->jsonify() << "/" << sequentialActivityEntries[ tokenAtSequentialPerformer ].empty() << std::endl;
    sequentialActivityEntries[tokenAtSequentialPerformer].emplace_back(
      std::move(token_ptr), std::move(request_ptr), std::move(decision)
    );
  }
  unevaluatedSequentialEntries.clear();
  
  // now find best activity to be entered for any sequential performer
  for ( auto& [ performerToken, decisionTuples ] : sequentialActivityEntries ) {
    assert( !performerToken->performing );
//std::cerr << "Performer: " << performerToken->jsonify() << std::endl;
    for ( auto& [ token_ptr, request_ptr, decision ] : decisionTuples ) {
      // Call decision->evaluate() and add the evaluation
      decision->evaluate();
//std::cerr << "Exclusive: " << decision->jsonify() << std::endl;
      decisionStore.addEvaluation(token_ptr, request_ptr, decision);
    }
    // best decision is infeasible, proceed with next performer
    if ( auto event = decisionStore.getBestDecision() ) {
      return event;
    }
  }

  return nullptr;
}

