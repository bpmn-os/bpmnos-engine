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
      std::optional<BPMNOS::Values> statusAttributes = systemState->getStatusAttributes( token->owner->root, token->node );
      std::optional<BPMNOS::Values> dataAttributes = systemState->getDataAttributes( token->owner->root, token->node );
      if ( statusAttributes.has_value() && dataAttributes.has_value() ) {
        return std::make_shared<ReadyEvent>( token.get(), std::move(statusAttributes.value()), std::move(dataAttributes.value()) );
      }
    }
  }
  return nullptr;
}
