#include "DeterministicTaskCompletion.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

DeterministicTaskCompletion::DeterministicTaskCompletion()
{
}

std::shared_ptr<Event> DeterministicTaskCompletion::dispatchEvent( const SystemState* systemState ) {
  for ( auto [time,token_ptr] : systemState->tokensAwaitingCompletionEvent ) {
    if ( time <= systemState->getTime() ) {
      if ( auto token = token_ptr.lock() )  {
        assert( token );
        return std::make_shared<CompletionEvent>(token.get());
      }
    }
  }
  return nullptr;
}
