#include "FirstMatchingMessageHandler.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstMatchingMessageHandler::FirstMatchingMessageHandler()
{
}

std::shared_ptr<Event> FirstMatchingMessageHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event ] : systemState->pendingMessageDeliveryEvents ) {
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
/*
std::unique_ptr<Event> FirstMatchingMessageHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr,messageHeader] : systemState->tokensAwaitingMessageDelivery ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      for ( auto candidate : token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates ) {
        if ( auto it = systemState->outbox.find(candidate); it != systemState->outbox.end() ) {
          for ( auto& [message_ptr] : it->second ) {
            auto message = message_ptr.lock();
            if ( message && message->matches(messageHeader) ) {
              return std::make_unique<MessageDeliveryEvent>(token.get(), message.get());
            }
          }
        }
      }
    }
  }
  return nullptr;
}
*/
