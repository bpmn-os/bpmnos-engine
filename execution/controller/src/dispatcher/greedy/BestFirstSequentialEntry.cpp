#include "BestFirstSequentialEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstSequentialEntry::BestFirstSequentialEntry( std::function<std::optional<double>(const Event* event)> evaluator )
  : GreedyDispatcher(evaluator)
{
}

void BestFirstSequentialEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest,
    Observable::Type::SequentialPerformerUpdate
  );
  GreedyDispatcher::connect(mediator);
}

std::shared_ptr<Event> BestFirstSequentialEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [ token_ptr, request_ptr, decision ] : decisionsWithoutEvaluation ) {
    assert(decision);
    if ( decision && !decision->expired() ) {
      auto token = token_ptr.lock();
      assert( token );
      auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get()));
      assert( tokenAtSequentialPerformer );
      if ( tokenAtSequentialPerformer->performing ) {
        pendingDecisionsWithoutEvaluation.emplace_back(token_ptr, request_ptr, decision);
      }
      else {
        evaluate( token_ptr, request_ptr, decision );
      }
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, decision_ptr ] : evaluatedDecisions ) {
    auto token = token_ptr.lock();
    assert( token );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get()));
    assert( tokenAtSequentialPerformer );
    if ( !tokenAtSequentialPerformer->performing ) {
      if( auto decision = decision_ptr.lock();
        decision && !decision->expired()
      )  {
        return decision;
      }
    }
  }

  return nullptr;
}

void BestFirstSequentialEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    assert(dynamic_cast<const DecisionRequest*>(observable));
    entryRequest(static_cast<const DecisionRequest*>(observable));
  }
  else if ( observable->getObservableType() == Observable::Type::SequentialPerformerUpdate ) {
    assert(dynamic_cast<const SequentialPerformerUpdate*>(observable));
    sequentialPerformerUpdate(static_cast<const SequentialPerformerUpdate*>(observable));
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

void BestFirstSequentialEntry::entryRequest(const DecisionRequest* request) {
  assert(request->token->node);
  if ( request->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
}

void BestFirstSequentialEntry::sequentialPerformerUpdate(const SequentialPerformerUpdate* update) {
  auto tokenAtSequentialPerformer = update->token;
  if ( tokenAtSequentialPerformer->performing ) {
    // performer has become busy
    for ( auto it = evaluatedDecisions.begin(); it != evaluatedDecisions.end(); ) {
      auto [ cost, token_ptr, request_ptr, decision_ptr ] = *it;
      auto token = token_ptr.lock();
      assert( token );
      if ( tokenAtSequentialPerformer == token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get())) ) {
        it = evaluatedDecisions.erase(it);
        pendingEvaluatedDecisions.emplace(cost, token_ptr, request_ptr, decision_ptr);
        continue;
      }
      ++it;
    }
    
    for ( auto it = decisionsWithoutEvaluation.begin(); it != decisionsWithoutEvaluation.end(); ) {
      auto [ token_ptr, request_ptr, decision ] = *it;
      auto token = token_ptr.lock();
      assert( token );
      if ( tokenAtSequentialPerformer == token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get())) ) {
        it = decisionsWithoutEvaluation.erase(it);        
        pendingDecisionsWithoutEvaluation.emplace_back(token_ptr, request_ptr, decision);
        continue;
      }
      ++it;
    }
  }
  else {
    // performer has become idle
    for ( auto it = pendingEvaluatedDecisions.begin(); it != pendingEvaluatedDecisions.end(); ) {
      auto [ cost, token_ptr, request_ptr, decision_ptr ] = *it;
      auto token = token_ptr.lock();
      assert( token );
      if ( tokenAtSequentialPerformer == token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get())) ) {
        it = pendingEvaluatedDecisions.erase(it);
        evaluatedDecisions.emplace(cost, token_ptr, request_ptr, decision_ptr);
        continue;
      }
      ++it;
    }
    
    for ( auto it = pendingDecisionsWithoutEvaluation.begin(); it != pendingDecisionsWithoutEvaluation.end(); ) {
      auto [ token_ptr, request_ptr, decision ] = *it;
      auto token = token_ptr.lock();
      assert( token );
      if ( tokenAtSequentialPerformer == token->owner->systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get())) ) {
        it = pendingDecisionsWithoutEvaluation.erase(it);
        decisionsWithoutEvaluation.emplace_back(token_ptr, request_ptr, decision);
        continue;
      }
      ++it;
    }
  }
}

