#include "MyopicMessageTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicMessageTaskTerminator::MyopicMessageTaskTerminator()
{
}

std::shared_ptr<Event> MyopicMessageTaskTerminator::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  // determine whether there is another decision pending
  if ( !otherEvents.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingTimer.empty() ) {
    return nullptr;
  }

  // all tokens have moved as far as they can
  
  // assume that no message can be delivered

  for ( auto& [token_ptr, event ] : receiveTaskEvents ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      // raise error at receive task
      return std::make_shared<ErrorEvent>(token.get());
    }
  }

  return nullptr;
}

void MyopicMessageTaskTerminator::notice(Event* event) {
  assert(event->token->node);
  if ( !dynamic_cast<MessageDeliveryEvent*>(event) ) {
    otherEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
  else if ( event->token->node->represents<BPMN::ReceiveTask>() ) {
    receiveTaskEvents.emplace_back(event->token->weak_from_this(), event->weak_from_this());
  }
};

/*
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
*/

