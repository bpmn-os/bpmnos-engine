#include "InstantExit.h"
#include "execution/engine/src/events/ExitEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExit::InstantExit()
{
}

std::shared_ptr<Event> InstantExit::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr] : systemState->pendingExitDecisions ) {
    if( auto request = request_ptr.lock() )  {
      assert( request );
      return std::make_shared<ExitEvent>(request->token);
    }
  }
  return nullptr;
}

