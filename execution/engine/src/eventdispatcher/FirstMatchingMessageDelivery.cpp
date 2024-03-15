#include "FirstMatchingMessageDelivery.h"
#include "execution/engine/src/events/MessageDeliveryDecision.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstMatchingMessageDelivery::FirstMatchingMessageDelivery()
{
}

std::shared_ptr<Event> FirstMatchingMessageDelivery::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event ] : systemState->pendingMessageDeliveryDecisions ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      for ( auto candidate : token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates ) {
        if ( auto it = systemState->outbox.find(candidate); it != systemState->outbox.end() ) {
          for ( auto& [message_ptr] : it->second ) {
            auto message = message_ptr.lock();
            if ( message && message->matches(event->recipientHeader) ) {
              event->message = message.get();
              return event;
//              return std::make_unique<MessageDeliveryEvent>(token.get(), message.get());
            }
          }
        }
      }
    }
  }
  return nullptr;
}

