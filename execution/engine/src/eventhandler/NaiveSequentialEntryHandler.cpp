#include "NaiveSequentialEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;

NaiveSequentialEntryHandler::NaiveSequentialEntryHandler()
{
}

std::shared_ptr<Event> NaiveSequentialEntryHandler::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, event_ptr] : sequentialEntryEvents ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      if ( auto event = event_ptr.lock() )  {
        auto tokenAtSequentialPerformer = systemState->tokenAtSequentialPerformer.at(const_cast<Token*>(token.get()));
        if ( !tokenAtSequentialPerformer->performing ) {
          return event;
        }
      }
    }
  }
  return nullptr;
}

void NaiveSequentialEntryHandler::notice(EntryEvent* event) {
  assert(event->token->node);
  if ( event->token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    sequentialEntryEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
};

/*
std::unique_ptr<Event> NaiveSequentialEntryHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr] : systemState->tokensAwaitingSequentialEntry ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      auto tokenAtSequentialPerformer = systemState->tokenAtSequentialPerformer.at(token.get());
      if ( !tokenAtSequentialPerformer->performing ) {
        return std::make_unique<EntryEvent>(token.get());
      }
    }
  }

  return nullptr;
}
*/
