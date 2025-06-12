#include "GreedyEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

GreedyEntry::GreedyEntry(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
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
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}


std::shared_ptr<Event> GreedyEntry::dispatchEvent( const SystemState* systemState ) {
//std::cout << "dispatchEvent" << std::endl;
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  std::unordered_map<const Token*, auto_list<std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > > sequentialActivityEntries;

  for (auto it = decisionsWithoutEvaluation.begin(); it != decisionsWithoutEvaluation.end(); ) {
    auto [ token_ptr, request_ptr, decision  ] = *it;
    assert(decision);
    if ( auto token = token_ptr.lock();
      token &&
      !token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
    ) {
      // evaluation of decision is independent of others
      auto reward = decision->evaluate();
//std::cerr << "Regular: " << decision->jsonify() << std::endl;
      addEvaluation(token_ptr, request_ptr, decision, reward);
      it = decisionsWithoutEvaluation.erase(it);

      if (  reward.has_value() ) {
        // dispatch feasible decision 
        return std::make_shared<EntryEvent>(decision->token);
      }
    }
    else {
      ++it;
    }
  }

  // all remaining entry decisions without evaluation require sequential performer
  for (auto it = decisionsWithoutEvaluation.begin(); it != decisionsWithoutEvaluation.end(); ) {
    auto [ token_ptr, request_ptr, decision  ] = std::move(*it);  // Move out the tuple to avoid dangling reference
    it = decisionsWithoutEvaluation.erase(it);
    assert(decision);
    if ( auto token = token_ptr.lock() ) {
      assert( token->node->parent );
      assert( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );
      // defer entry decision for sequential activities
      auto tokenAtSequentialPerformer = token->getSequentialPerformerToken();
//std::cerr << "Defer: " << token->jsonify() << "/" << sequentialActivityEntries[ tokenAtSequentialPerformer ].empty() << std::endl;
      sequentialActivityEntries[ tokenAtSequentialPerformer ].emplace_back( token_ptr, request_ptr, decision );
    }
  }
  
  // all evaluated decisions are infeasible unless a previously dispatched decision was not deployed
  for ( auto decisionTuple : evaluatedDecisions ) {
    constexpr std::size_t last = std::tuple_size<decltype(decisionTuple)>::value - 1;
    std::weak_ptr<Event>& event_ptr = std::get<last>(decisionTuple);
    if ( auto event = event_ptr.lock();
      event && std::get<0>(decisionTuple) < std::numeric_limits<double>::max()
    ) {
//std::cerr << "\nBest (old) decision " << event->jsonify() << " evaluated with " << std::get<0>(decisionTuple) << std::endl;
      // Warning: event is dispatched even if it is no longer the best due to data updates since undeployed dispatch
      return event;
    }
    else {
      // best decision is infeasible, no need to inspect others
      break;
    }
  }
  
  // now find best activity to be entered for any sequential performer
  for ( auto& [ performerToken, decisionTuples ] : sequentialActivityEntries ) {
    assert( !performerToken->performing );
//std::cerr << "Performer: " << performerToken->jsonify() << std::endl;
    for ( auto& [ token_ptr, request_ptr, decision ] : decisionTuples ) {
      // Call decision->evaluate() and add the evaluation
      auto reward = decision->evaluate();
//std::cerr << "Exclusive: " << decision->jsonify() << std::endl;
      addEvaluation(token_ptr, request_ptr, decision, reward);
    }
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
        // best decision is infeasible, proceed with next performer
        break;
      }
    }
  }

  return nullptr;
}

