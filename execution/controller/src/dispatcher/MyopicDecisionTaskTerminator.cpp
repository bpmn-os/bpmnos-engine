#include "MyopicDecisionTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicDecisionTaskTerminator::MyopicDecisionTaskTerminator()
{
}

std::shared_ptr<Event> MyopicDecisionTaskTerminator::dispatchEvent( const SystemState* systemState ) {
  // assume that feasible choices are already made by an appropriate handler
  for ( auto& [token_ptr, event] : systemState->pendingChoiceDecisions ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      // raise error at decision task without prior decision
      return std::make_shared<ErrorEvent>(token.get());
    }
  }
  return nullptr;
}

