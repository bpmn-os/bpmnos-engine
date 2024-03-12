#include "BestFirstSequentialEntryHandler.h"
#include "execution/engine/src/Engine.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestFirstSequentialEntryHandler::BestFirstSequentialEntryHandler()
{
}

void BestFirstSequentialEntryHandler::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Observable::Type::EntryEvent,
    Observable::Type::SequentialPerformerUpdate
  );
  EventHandler::subscribe(engine);
}

std::shared_ptr<Event> BestFirstSequentialEntryHandler::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
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

void BestFirstSequentialEntryHandler::notice(const Observable* observable) {
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

void BestFirstSequentialEntryHandler::noticeEntryEvent(const EntryEvent* event) {
  assert(event->token->node);
  auto token = const_cast<Token*>(event->token);
  if ( event->token->node->parent->represents<const BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    assert( token->owner->systemState->tokenAtSequentialPerformer.find(token) != token->owner->systemState->tokenAtSequentialPerformer.end() );
    auto tokenAtSequentialPerformer = token->owner->systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      // skip evaluation of event
      tokensAtBusyPerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( 0, token->weak_from_this(), const_cast<EntryEvent*>(event)->weak_from_this() );
    }
    else {
      // evaluate event
      tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( evaluate(event), token->weak_from_this(), const_cast<EntryEvent*>(event)->weak_from_this() );
    }
  }
}

void BestFirstSequentialEntryHandler::noticeSequentialPerformerUpdate(const SequentialPerformerUpdate* update) {
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
      for ( auto [cost, token_ptr, event_ptr] : it->second ) {
        if ( auto event = event_ptr.lock() ) {
          // re-evaluate event
          cost = evaluate( (const EntryEvent*)event.get()); 
          tokensAtIdlePerformers[tokenAtSequentialPerformer->weak_from_this()].emplace( cost, token_ptr, event_ptr );
        }
      }
      tokensAtBusyPerformers.erase(it);
    }
  }
}

BPMNOS::number BestFirstSequentialEntryHandler::evaluate(const EntryEvent* event) {
  auto extensionElements = event->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  Values status = event->token->status;
  BPMNOS::number cost = extensionElements->getContributionToObjective(status);
  extensionElements->applyOperators(status);
  cost -= extensionElements->getContributionToObjective(status);
  return cost;
}
