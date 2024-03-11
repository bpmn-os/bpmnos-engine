#include "NaiveSequentialEntryHandler.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

NaiveSequentialEntryHandler::NaiveSequentialEntryHandler()
{
}

void NaiveSequentialEntryHandler::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Observable::Type::EntryEvent,
    Observable::Type::SequentialPerformerUpdate
  );
  EventHandler::subscribe(engine);
}

std::shared_ptr<Event> NaiveSequentialEntryHandler::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
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

void NaiveSequentialEntryHandler::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::EntryEvent ) {
    noticeEntryEvent(static_cast<const EntryEvent*>(observable));
  }
  else if ( observable->getObservableType() == Observable::Type::SequentialPerformerUpdate ) {
    noticeSequentialPerformerUpdate(static_cast<const SequentialPerformerUpdate*>(observable));
  }
  else {
    assert(!"Illegal observable type");
  }
}

void NaiveSequentialEntryHandler::noticeEntryEvent(const EntryEvent* event) {
  assert(event->token->node);
  auto token = const_cast<Token*>(event->token);
  if ( event->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    assert( token->owner->systemState->tokenAtSequentialPerformer.find(token) != token->owner->systemState->tokenAtSequentialPerformer.end() );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()].emplace_back( token->weak_from_this(), const_cast<EntryEvent*>(event)->weak_from_this() );
    }
    else {
      tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace_back( token->weak_from_this(), const_cast<EntryEvent*>(event)->weak_from_this() );
    }
  }
}

void NaiveSequentialEntryHandler::noticeSequentialPerformerUpdate(const SequentialPerformerUpdate* update) {
  auto tokenAtSequentialPerformer = update->token;
  if ( tokenAtSequentialPerformer->performing ) {
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

