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
      std::optional<BPMNOS::Values> values = systemState->getStatusAttributes( token->owner->root, token->node );
      std::optional<BPMNOS::Values> data = systemState->getDataAttributes( token->owner->root, token->node );
      if ( values.has_value() && data.has_value() ) {
        return std::make_shared<ReadyEvent>( token.get(), std::move(values.value()), std::move(data.value()) );
      }
/*
      BPMNOS::Values values;
      BPMNOS::Values data;
      if ( systemState->assumedTime ) {
        values = systemState->scenario->getAnticipatedValues(token->node, token->status, systemState->currentTime );
        data = systemState->scenario->getAnticipatedData(token->node, token->status, systemState->currentTime );
      }
      else {
        auto knownValues = systemState->scenario->getKnownValues(token->node, token->status, systemState->currentTime );
        auto knownData = systemState->scenario->getKnownData(token->node, token->status, systemState->currentTime );
        if ( knownValues && knownData ) {
          values = std::move( knownValues.value() );
          data = std::move( knownData.value() );
        }
        else {
          continue;
        }
      }
      return std::make_shared<ReadyEvent>( token.get(), values, data );
*/
    }
  }
  return nullptr;
}
