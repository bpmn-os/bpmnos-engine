#include "InstantEntryHandler.h"
#include "execution/engine/src/events/EntryEvent.h"

using namespace BPMNOS::Execution;

InstantEntryHandler::InstantEntryHandler()
{
}

std::unique_ptr<Event> InstantEntryHandler::fetchEvent( const SystemState* systemState ) {
  if ( systemState->tokensAwaitingRegularEntryEvent.size() ) {
    return std::make_unique<EntryEvent>(systemState->tokensAwaitingRegularEntryEvent.front());
  }
  return nullptr;
}

