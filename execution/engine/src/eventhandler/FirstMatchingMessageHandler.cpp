#include "FirstMatchingMessageHandler.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "model/parser/src/extensionElements/Message.h"
#include <iostream>

using namespace BPMNOS::Execution;

FirstMatchingMessageHandler::FirstMatchingMessageHandler()
{
}

std::unique_ptr<Event> FirstMatchingMessageHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr,messageHeader] : systemState->tokensAwaitingMessageDelivery ) {
    if( auto token = token_ptr.lock() )  {
      auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::Message>();
      for ( auto candidate : messageDefinition->candidates ) {
        if ( auto it = systemState->messages.find(candidate); it != systemState->messages.end() ) {
          for ( auto& message : it->second ) {
            if ( message->matches(messageHeader) ) {
              return std::make_unique<MessageDeliveryEvent>(token.get(), message.get());
            }
          }
        }
      }
    }
  }
  return nullptr;
}

