#include "InstantExit.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExit::InstantExit()
{
}

std::shared_ptr<Event> InstantExit::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event] : systemState->pendingExitDecisions ) {
    return event;
  }
  return nullptr;
}

