#include "DeterministicTaskCompletionHandler.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

DeterministicTaskCompletionHandler::DeterministicTaskCompletionHandler()
{
}

std::shared_ptr<Event> DeterministicTaskCompletionHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto [time, token_ptr, event] : systemState->pendingCompletionEvents ) {
    if ( time <= systemState->getTime() ) {
      if ( auto token = token_ptr.lock() ) {
        assert( token );
        return event;
      }
    }
  }
  return nullptr;
}

/*
std::unique_ptr<Event> DeterministicTaskCompletionHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto [time,token_ptr,updatedStatus] : systemState->tokensAwaitingTaskCompletion ) {
    if ( auto token = token_ptr.lock() )  {
      assert( token );
      if ( time <= systemState->getTime() ) {
        return std::make_unique<CompletionEvent>(token.get(),std::move(updatedStatus));
      }
    }
  }
  return nullptr;
}
*/
