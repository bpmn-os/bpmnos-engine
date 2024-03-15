#include "BestFirstSequentialEntry.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstSequentialEntry::BestFirstSequentialEntry()
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
      for ( auto [ cost, token_ptr, event_ptr] : it->second ) {
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

void BestFirstSequentialEntry::notice(const Observable* observable) {
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

void BestFirstSequentialEntry::entryRequest(const EntryDecision* event) {
  assert(event->token->node);
  auto token = const_cast<Token*>(event->token);
  if ( event->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    assert( token->owner->systemState->tokenAtSequentialPerformer.find(token) != token->owner->systemState->tokenAtSequentialPerformer.end() );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      // skip evaluation of event
      tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( 0, token->weak_from_this(), const_cast<EntryDecision*>(event)->weak_from_this() );
    }
    else {
      // evaluate event
      tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( cost(event), token->weak_from_this(), const_cast<EntryDecision*>(event)->weak_from_this() );
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
      for ( auto [_, token_ptr, event_ptr] : it->second ) {
        if ( auto token = token_ptr.lock() ) {
          if ( auto event = event_ptr.lock() ) {
            // re-evaluate event
            tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( cost((const EntryDecision*)event.get()), token_ptr, event_ptr );
          }
        }
      }
      tokensAtBusyPerformers.erase(it);
    }
  }
}

BPMNOS::number BestFirstSequentialEntry::cost(const EntryDecision* event) {
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  BPMNOS::number cost = extensionElements->getObjective(status);
  extensionElements->applyOperators(status);
  cost -= extensionElements->getObjective(status);
  return cost;
}
