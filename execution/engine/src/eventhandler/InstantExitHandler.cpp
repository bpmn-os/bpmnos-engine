#include "InstantExitHandler.h"
#include "execution/engine/src/events/ExitEvent.h"

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::unique_ptr<Event> InstantExitHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto token_ptr : const_cast<SystemState*>(systemState)->tokensAwaitingExitEvent ) {
    if ( auto token = token_ptr.lock() )  {
      return std::make_unique<ExitEvent>(token.get());
    }
  }
/*
  if ( systemState->tokensAwaitingExitEvent.size() ) {
    return std::make_unique<ExitEvent>(systemState->tokensAwaitingExitEvent.front());
  }
*/
  return nullptr;
}

