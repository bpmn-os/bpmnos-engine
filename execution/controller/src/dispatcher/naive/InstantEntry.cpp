#include "InstantEntry.h"
#include "execution/engine/src/events/EntryEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantEntry::InstantEntry()
{
}

std::shared_ptr<Event> InstantEntry::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr] : systemState->pendingEntryDecisions ) {
    if( auto request = request_ptr.lock() )  {
      assert( request );
      return std::make_shared<EntryEvent>(request->token);
    }
  }
  return nullptr;
}

