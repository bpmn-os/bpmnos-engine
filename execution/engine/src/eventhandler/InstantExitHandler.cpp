#include "InstantExitHandler.h"
#include "execution/engine/src/events/ExitEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::shared_ptr<Event> InstantExitHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event] : systemState->pendingExitEvents ) {
    return event;
  }
  return nullptr;
}

/*
std::unique_ptr<Event> InstantExitHandler::fetchEvent( const SystemState* systemState ) {
  
  for ( auto& [token_ptr] : systemState->tokensAwaitingExit ) {
    if ( auto token = token_ptr.lock() )  {
      assert( token );
      return std::make_unique<ExitEvent>(token.get());
    }
  }
  return nullptr;
}
*/
