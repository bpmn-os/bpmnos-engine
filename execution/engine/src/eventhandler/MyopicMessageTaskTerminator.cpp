#include "MyopicMessageTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicMessageTaskTerminator::MyopicMessageTaskTerminator()
{
}

std::unique_ptr<Event> MyopicMessageTaskTerminator::fetchEvent( const SystemState* systemState ) {
  // determine whether there is another decision pending
  if ( !systemState->tokensAwaitingParallelEntry.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingSequentialEntry.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingChoice.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingExit.empty() ) {
    return nullptr;
  }

  // only message delivery decisions are pending

  if ( !systemState->tokensAwaitingReadyEvent.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingTaskCompletion.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingTimer.empty() ) {
    return nullptr;
  }

  // all tokens have moved as far as they can
  
  // assume that no message can be delivered

  for ( auto& [token_ptr,messageHeader] : systemState->tokensAwaitingMessageDelivery ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      if ( token->node->represents<BPMN::ReceiveTask>() ) {
        // raise error at receive task
        return std::make_unique<ErrorEvent>(token.get());
      }
    }
  }

  for ( auto& [token,message_ptr] : systemState->messageAwaitingDelivery ) {
    // raise error at send task
    return std::make_unique<ErrorEvent>(token);
  }
  return nullptr;
}

