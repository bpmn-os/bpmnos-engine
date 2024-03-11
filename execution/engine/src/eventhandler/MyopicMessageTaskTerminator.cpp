#include "MyopicMessageTaskTerminator.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include "execution/engine/src/Engine.h"
#include <cassert>

using namespace BPMNOS::Execution;

MyopicMessageTaskTerminator::MyopicMessageTaskTerminator()
{
}

void MyopicMessageTaskTerminator::subscribe(Engine* engine) {
  engine->addSubscriber(this,
    Execution::Observable::Type::ReadyEvent,
    Execution::Observable::Type::EntryEvent,
    Execution::Observable::Type::ChoiceEvent,
    Execution::Observable::Type::CompletionEvent,
    Execution::Observable::Type::ExitEvent,
    Execution::Observable::Type::MessageDeliveryEvent
  );
  EventHandler::subscribe(engine);
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

  // no receive task is pending, raise error at remaining send tasks
  for ( auto& [token,message_ptr] : systemState->messageAwaitingDelivery ) {
    // raise error at send task
    return std::make_shared<ErrorEvent>(token);
  }

  return nullptr;
}

void MyopicMessageTaskTerminator::notice(const Observable* observable) {
  if ( const Event* event = dynamic_cast<const MessageDeliveryEvent*>(observable);
    event &&
    event->token->node->represents<const BPMN::ReceiveTask>()
  ) {
    receiveTaskEvents.emplace_back(event->token->weak_from_this(), const_cast<Event*>(event)->weak_from_this());
  }
  else {
    event = static_cast<const Event*>(observable);
    otherEvents.emplace_back(event->token->weak_from_this(), const_cast<Event*>(event)->weak_from_this());
  }
}

