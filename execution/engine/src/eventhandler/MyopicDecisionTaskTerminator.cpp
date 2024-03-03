#include "MyopicDecisionTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicDecisionTaskTerminator::MyopicDecisionTaskTerminator()
{
}

std::unique_ptr<Event> MyopicDecisionTaskTerminator::fetchEvent( const SystemState* systemState ) {
  // assume that feasible choices are already made by an appropriate handler
  for ( auto& [ token_ptr ] : systemState->tokensAwaitingChoice ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      // raise error at decision task without prior decision
      return std::make_unique<ErrorEvent>(token.get());
    }
  }
  return nullptr;
}

