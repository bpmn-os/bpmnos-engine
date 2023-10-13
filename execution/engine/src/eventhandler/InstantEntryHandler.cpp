#include "InstantEntryHandler.h"
#include "execution/engine/src/events/ExitEvent.h"

using namespace BPMNOS::Execution;

InstantEntryHandler::InstantEntryHandler()
{
}

std::unique_ptr<Event> InstantEntryHandler::fetchEvent( const SystemState* systemState ) {
  if ( systemState->awaitingRegularEntry.size() ) {
    return std::make_unique<ExitEvent>(systemState->awaitingRegularEntry[0]);
  }
  return nullptr;
}

