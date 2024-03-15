#include "FirstComeFirstServedSequentialEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstComeFirstServedSequentialEntry::FirstComeFirstServedSequentialEntry()
{
}

void FirstComeFirstServedSequentialEntry::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::EntryRequest,
    Observable::Type::SequentialPerformerUpdate
  );
  EventDispatcher::connect(mediator);
}

std::shared_ptr<Event> FirstComeFirstServedSequentialEntry::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto it = tokensAtIdlePerformers.begin(), next_it = it; it != tokensAtIdlePerformers.end(); it = next_it) {
    next_it++;
    if ( it->first.expired() ) {
      tokensAtIdlePerformers.erase(it);
    }
    else {
      for ( auto& [token_ptr, event_ptr] : it->second ) {
        if( auto token = token_ptr.lock() )  {
          assert( token );
          if ( auto event = event_ptr.lock() )  {
            return event;
          }
        }
      }
    }
  }
  return nullptr;
}

void FirstComeFirstServedSequentialEntry::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryRequest ) {
    entryRequest(static_cast<const EntryDecision*>(observable));
  }
  else if ( observable->getObservableType() == Observable::Type::SequentialPerformerUpdate ) {
    sequentialPerformerUpdate(static_cast<const SequentialPerformerUpdate*>(observable));
  }
  else {
    assert(!"Illegal observable type");
  }
}

void FirstComeFirstServedSequentialEntry::entryRequest(const EntryDecision* event) {
  assert(event->token->node);
  auto token = const_cast<Token*>(event->token);
  if ( event->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    assert( token->owner->systemState->tokenAtSequentialPerformer.find(token) != token->owner->systemState->tokenAtSequentialPerformer.end() );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()].emplace_back( token->weak_from_this(), const_cast<EntryDecision*>(event)->weak_from_this() );
    }
    else {
      tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace_back( token->weak_from_this(), const_cast<EntryDecision*>(event)->weak_from_this() );
    }
  }
}

void FirstComeFirstServedSequentialEntry::sequentialPerformerUpdate(const SequentialPerformerUpdate* update) {
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
      if ( !it->second.empty() ) {
        tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()] = std::move(it->second);
      }
      tokensAtBusyPerformers.erase(it);
    }
  }
}

