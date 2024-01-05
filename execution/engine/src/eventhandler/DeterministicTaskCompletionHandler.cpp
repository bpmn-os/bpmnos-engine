#include "DeterministicTaskCompletionHandler.h"
#include "execution/engine/src/events/TaskCompletionEvent.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

DeterministicTaskCompletionHandler::DeterministicTaskCompletionHandler()
{
}

std::unique_ptr<Event> DeterministicTaskCompletionHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto [time,token_ptr,updatedStatus] : systemState->tokensAwaitingTaskCompletionEvent ) {
    if ( auto token = token_ptr.lock() )  {
      if ( time <= systemState->getTime() ) {
        return std::make_unique<TaskCompletionEvent>(token.get(),updatedStatus);
      }
    }
  }
  return nullptr;
}

