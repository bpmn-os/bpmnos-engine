#include "InstantExitHandler.h"
#include "execution/engine/src/events/ExitEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::unique_ptr<Event> InstantExitHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr] : systemState->tokensAwaitingExitEvent ) {
    if ( auto token = token_ptr.lock() )  {
      assert( token );
      return std::make_unique<ExitEvent>(token.get());
    }
  }
  return nullptr;
}

