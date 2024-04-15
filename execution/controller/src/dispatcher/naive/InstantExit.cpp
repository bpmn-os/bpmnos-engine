#include "InstantExit.h"
#include "execution/engine/src/events/ExitEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExit::InstantExit()
{
}

std::shared_ptr<Event> InstantExit::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr] : systemState->pendingExitEvents ) {
    if ( auto token = token_ptr.lock() ) {
      return std::make_shared<ExitEvent>(token.get());
    }
  }
  return nullptr;
}

