#include "InstantEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntryHandler::InstantEntryHandler()
{
}

std::unique_ptr<Event> InstantEntryHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr] : systemState->tokensAwaitingParallelEntry ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      return std::make_unique<EntryEvent>(token.get());
    }
  }

  return nullptr;
}

