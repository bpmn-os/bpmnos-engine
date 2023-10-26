#include "InstantExitHandler.h"
#include "execution/engine/src/events/ExitEvent.h"

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::unique_ptr<Event> InstantExitHandler::fetchEvent( const SystemState* systemState ) {
  if ( systemState->tokensAwaitingExitEvent.size() ) {
    return std::make_unique<ExitEvent>(systemState->tokensAwaitingExitEvent.front());
  }
  return nullptr;
}

