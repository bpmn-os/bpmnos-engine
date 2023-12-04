#include "ReadyHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"

using namespace BPMNOS::Execution;

ReadyHandler::ReadyHandler()
{
}

std::unique_ptr<Event> ReadyHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto token_ptr : const_cast<SystemState*>(systemState)->tokensAwaitingReadyEvent ) {
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
/*
  if ( systemState->tokensAwaitingReadyEvent.size() ) {
    auto& token = systemState->tokensAwaitingReadyEvent.front(); 
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
*/
  return nullptr;
}

