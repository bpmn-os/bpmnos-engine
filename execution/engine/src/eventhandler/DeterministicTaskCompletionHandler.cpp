#include "DeterministicTaskCompletionHandler.h"
#include "execution/engine/src/events/TaskCompletionEvent.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

DeterministicTaskCompletionHandler::DeterministicTaskCompletionHandler()
{
}

std::unique_ptr<Event> DeterministicTaskCompletionHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [time,token] : systemState->tokensAwaitingTaskCompletionEvent ) {
    if ( time <= systemState->getTime() ) {
      std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues; // TODO
      return std::make_unique<TaskCompletionEvent>(token,updatedValues);
    }
  }
  return nullptr;
}

