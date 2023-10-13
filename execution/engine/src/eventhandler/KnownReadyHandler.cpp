#include "KnownReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

KnownReadyHandler::KnownReadyHandler()
{
}

std::unique_ptr<Event> KnownReadyHandler::fetchEvent( const SystemState& systemState ) {
  for ( auto token : systemState.awaitingReady ) {
    if ( systemState.scenario->addKnownValues(token->node, token->status, systemState.currentTime) ) {
      return std::make_unique<ReadyEvent>(token);
    }
  }
  return nullptr;
}

