#include "InstantExitHandler.h"
#include "execution/engine/src/events/ExitEvent.h"

using namespace BPMNOS::Execution;

InstantExitHandler::InstantExitHandler()
{
}

std::unique_ptr<Event> InstantExitHandler::fetchEvent( const SystemState* systemState ) {
  if ( systemState->awaitingExit.size() ) {
    return std::make_unique<ExitEvent>(systemState->awaitingExit[0]);
  }
  return nullptr;
}

