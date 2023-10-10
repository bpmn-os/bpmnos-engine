#include "DeterministicCompletionHandler.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

DeterministicCompletionHandler::DeterministicCompletionHandler()
{
}

std::unique_ptr<Event> DeterministicCompletionHandler::fetchEvent( const SystemState& systemState ) {
  for ( auto token : systemState.awaitingCompletion ) {
    if ( token->status[BPMNOS::Model::Status::Index::Timestamp] <= systemState.getTime() ) {  
      return std::make_unique<CompletionEvent>(token);
    }
  }
  return nullptr;
}

