#include "BestFirstSequentialEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstSequentialEntry::BestFirstSequentialEntry( std::function<std::optional<double>(const Event* event)> evaluator )
  : evaluator(evaluator)
{
}

void BestFirstSequentialEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest,
    Observable::Type::SequentialPerformerUpdate
  );
  EventDispatcher::connect(mediator);
}

std::shared_ptr<Event> BestFirstSequentialEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto it = tokensAtIdlePerformers.begin(), next_it = it; it != tokensAtIdlePerformers.end(); it = next_it) {
    next_it++;
    if ( it->first.expired() ) {
      tokensAtIdlePerformers.erase(it);
    }
    else {
      for ( auto [ cost, token_ptr, request_ptr, decision ] : it->second ) {
        if( auto token = token_ptr.lock() )  {
          assert( token );
          if ( request_ptr.lock() && decision->evaluation.has_value() )  {
            // request is still valid and evaluation of decision gave a value
            return decision;
          }
        }
      }
    }
  }
  return nullptr;
}

void BestFirstSequentialEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    entryRequest(static_cast<const DecisionRequest*>(observable));
  }
  else if ( observable->getObservableType() == Observable::Type::SequentialPerformerUpdate ) {
    sequentialPerformerUpdate(static_cast<const SequentialPerformerUpdate*>(observable));
  }
  else {
    assert(!"Illegal observable type");
  }
}

void BestFirstSequentialEntry::entryRequest(const DecisionRequest* request) {
  assert(request->token->node);
  auto token = const_cast<Token*>(request->token);
  if ( request->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto decision = std::make_shared<EntryDecision>(request->token, evaluator);
    assert( token->owner->systemState->tokenAtSequentialPerformer.find(token) != token->owner->systemState->tokenAtSequentialPerformer.end() );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( 0, token->weak_from_this(), request->weak_from_this(), decision );
    }
    else {
      tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), token->weak_from_this(), request->weak_from_this(), decision );
    }
  }
}

void BestFirstSequentialEntry::sequentialPerformerUpdate(const SequentialPerformerUpdate* update) {
  auto tokenAtSequentialPerformer = update->token;
  if ( tokenAtSequentialPerformer->performing ) {
    // perfomer has just become busy
    if ( auto it = tokensAtIdlePerformers.find(tokenAtSequentialPerformer->weak_from_this());
      it != tokensAtIdlePerformers.end()
    ) {
      if ( !it->second.empty() ) {
        tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()] = std::move(it->second);
      }
      tokensAtIdlePerformers.erase(it);
    }
  }
  else {
    // perfomer has just become idle
    if ( auto it = tokensAtBusyPerformers.find(tokenAtSequentialPerformer->weak_from_this());
      it != tokensAtBusyPerformers.end()
    ) {
      for ( auto [_, token_ptr, request_ptr, decision ] : it->second ) {
        if ( auto token = token_ptr.lock() ) {
          if ( auto request = request_ptr.lock() ) {
            // re-evaluate event
            decision->evaluate();
            tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), token_ptr, request_ptr, decision );
          }
        }
      }
      tokensAtBusyPerformers.erase(it);
    }
  }
}

