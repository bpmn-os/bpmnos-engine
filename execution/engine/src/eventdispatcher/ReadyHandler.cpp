#include "ReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

ReadyHandler::ReadyHandler()
{
}

std::shared_ptr<Event> ReadyHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr] : systemState->tokensAwaitingReadyEvent ) {
    if ( auto token = token_ptr.lock() )  {
      assert( token );
      if ( systemState->assumedTime ) {
        return std::make_shared<ReadyEvent>( token.get(), systemState->scenario->getAnticipatedValues(token->node, token->status, systemState->currentTime ) );
      }
      else {
        auto values = systemState->scenario->getKnownValues(token->node, token->status, systemState->currentTime );
        if ( values ) {
          return std::make_shared<ReadyEvent>( token.get(),std::move( values.value() ) );
        }
      }
    }
  }
/*
  for ( auto& [token_ptr, event] : systemState->pendingReadyEvents ) {
    if ( auto token = token_ptr.lock() )  {
      assert( token );
      if ( systemState->assumedTime ) {
        event->values = systemState->scenario->getAnticipatedValues(token->node, token->status, systemState->currentTime );
        return event;
      }
      else {
        auto values = systemState->scenario->getKnownValues(token->node, token->status, systemState->currentTime );
        if ( values ) {
          event->values = std::move( values.value() );
          return event;
        }
      }
    }
  }
*/
  return nullptr;
}
