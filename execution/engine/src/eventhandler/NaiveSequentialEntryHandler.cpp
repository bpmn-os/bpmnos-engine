#include "NaiveSequentialEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

NaiveSequentialEntryHandler::NaiveSequentialEntryHandler()
{
}

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

