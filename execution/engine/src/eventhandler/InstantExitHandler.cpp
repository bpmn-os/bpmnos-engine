#include "InstantExitHandler.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::shared_ptr<Event> InstantExitHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event] : systemState->pendingExitDecisions ) {
    return event;
  }
  return nullptr;
}

