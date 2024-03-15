#include "MyopicMessageTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include "execution/engine/src/Engine.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicMessageTaskTerminator::MyopicMessageTaskTerminator()
{
}

void MyopicMessageTaskTerminator::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Observable::Type::EntryRequest,
    Observable::Type::ChoiceRequest,
    Observable::Type::ExitRequest,
    Observable::Type::MessageDeliveryRequest
  );
  EventDispatcher::connect(mediator);
}


std::shared_ptr<Event> MyopicMessageTaskTerminator::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  // determine whether there is another decision pending
  if ( !otherDecisions.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingTimer.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingReadyEvent.empty() ) {
    return nullptr;
  }

  if ( !systemState->tokensAwaitingCompletionEvent.empty() ) {
    return nullptr;
  }

  // all tokens have moved as far as they can
  
  // assume that no message can be delivered

  for ( auto& [token_ptr, decision ] : messageDeliveryDecisions ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      // raise error at receive task
      return std::make_shared<ErrorEvent>(token.get());
    }
  }

  // no receive task is pending, raise error at remaining send tasks
  for ( auto& [token,message_ptr] : systemState->messageAwaitingDelivery ) {
    // raise error at send task
    return std::make_shared<ErrorEvent>(token);
  }

  return nullptr;
}

void MyopicMessageTaskTerminator::notice(const Observable* observable) {
  if ( const Decision* decision = dynamic_cast<const MessageDeliveryDecision*>(observable);
    decision &&
    decision->token->node->represents<const BPMN::ReceiveTask>()
  ) {
    messageDeliveryDecisions.emplace_back(decision->token->weak_from_this(), const_cast<Decision*>(decision)->weak_from_this());
  }
  else {
    decision = static_cast<const Decision*>(observable);
    otherDecisions.emplace_back(decision->token->weak_from_this(), const_cast<Decision*>(decision)->weak_from_this() );
  }
}

