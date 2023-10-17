#include "ReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

ReadyHandler::ReadyHandler()
{
}

std::unique_ptr<Event> ReadyHandler::fetchEvent( const SystemState* systemState ) {
  if ( systemState->awaitingReady.size() ) {
    auto& token = systemState->awaitingReady.front(); 
    if ( systemState->assumedTime ) {
      auto values = systemState->scenario->getAnticipatedValues(token->node, token->status, systemState->currentTime );
      return std::make_unique<ReadyEvent>(token,values);
    }
    else {
      auto values = systemState->scenario->getKnownValues(token->node, token->status, systemState->currentTime );
      if ( values ) {
        return std::make_unique<ReadyEvent>(token,values.value());
      }
    }
  }
  return nullptr;
}

