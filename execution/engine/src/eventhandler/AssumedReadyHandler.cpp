#include "AssumedReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

AssumedReadyHandler::AssumedReadyHandler()
{
}

std::unique_ptr<Event> AssumedReadyHandler::fetchEvent( const SystemState& systemState ) {
  if ( systemState.awaitingReady.size() ) {
    auto& token = systemState.awaitingReady.front(); 
    systemState.scenario->getAssumedValues(token->node, token->status, systemState.getTime());
    return std::make_unique<ReadyEvent>(token);
  }
  return nullptr;
}

