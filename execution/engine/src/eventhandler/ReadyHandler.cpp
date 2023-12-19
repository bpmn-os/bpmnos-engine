#include "ReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

ReadyHandler::ReadyHandler()
{
}

std::unique_ptr<Event> ReadyHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto token_ptr : systemState->tokensAwaitingReadyEvent ) {
    if ( auto token = token_ptr.lock() )  {
      if ( systemState->assumedTime ) {
        auto values = systemState->scenario->getAnticipatedValues(token->node, token->status, systemState->currentTime );
        return std::make_unique<ReadyEvent>(token.get(),values);
      }
      else {
        auto values = systemState->scenario->getKnownValues(token->node, token->status, systemState->currentTime );
        if ( values ) {
          return std::make_unique<ReadyEvent>(token.get(),values.value());
        }
      }
    }
  }
  return nullptr;
}

