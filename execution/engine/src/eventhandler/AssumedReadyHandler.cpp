#include "AssumedReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

AssumedReadyHandler::AssumedReadyHandler()
{
}

std::unique_ptr<Event> AssumedReadyHandler::fetchEvent( const SystemState& systemState ) {
  if ( systemState.awaitingReady.size() ) {
    auto& token = systemState.awaitingReady.front(); 
    systemState.scenario->addAssumedValues(token->node, token->status, systemState.currentTime, systemState.assumedTime.value() );
    return std::make_unique<ReadyEvent>(token);
  }
  return nullptr;
}

