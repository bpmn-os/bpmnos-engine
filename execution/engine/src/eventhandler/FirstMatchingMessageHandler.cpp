#include "FirstMatchingMessageHandler.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"
#include <iostream>
#include <cassert>

using namespace BPMNOS::Execution;

FirstMatchingMessageHandler::FirstMatchingMessageHandler()
{
}

std::unique_ptr<Event> FirstMatchingMessageHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr,messageHeader] : systemState->tokensAwaitingMessageDelivery ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      for ( auto candidate : token->node->extensionElements->as<BPMNOS::Model::Status>()->messageCandidates ) {
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

