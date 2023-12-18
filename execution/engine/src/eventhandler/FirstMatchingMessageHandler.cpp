#include "FirstMatchingMessageHandler.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"

using namespace BPMNOS::Execution;

FirstMatchingMessageHandler::FirstMatchingMessageHandler()
{
}

std::unique_ptr<Event> FirstMatchingMessageHandler::fetchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr,messageHeader] : const_cast<SystemState*>(systemState)->tokensAwaitingMessageDelivery ) {
    if( auto token = token_ptr.lock() )  {
//      return std::make_unique<MessageDeliveryEvent>(token.get(), nullptr);
    }
  }
  return nullptr;
}

