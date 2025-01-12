#include "MyopicDecisionTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicDecisionTaskTerminator::MyopicDecisionTaskTerminator()
{
}

std::shared_ptr<Event> MyopicDecisionTaskTerminator::dispatchEvent( const SystemState* systemState ) {
  // assume that feasible choices are already made by an appropriate handler
  for ( auto& [token_ptr, request_ptr] : systemState->pendingChoiceDecisions ) {
    if( auto request = request_ptr.lock() )  {
      assert( request );
      // raise error at decision task without prior decision
      return std::make_shared<ErrorEvent>(request->token);
    }
  }
  return nullptr;
}

