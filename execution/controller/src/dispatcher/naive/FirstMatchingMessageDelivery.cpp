#include "FirstMatchingMessageDelivery.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/events/MessageDeliveryDecision.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstMatchingMessageDelivery::FirstMatchingMessageDelivery()
{
}

void FirstMatchingMessageDelivery::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Execution::Observable::Type::MessageDeliveryRequest);
  EventDispatcher::connect(mediator);
}

void FirstMatchingMessageDelivery::notice(const Observable* observable) {
  assert(dynamic_cast<const DecisionRequest*>(observable));
  auto request = static_cast<const DecisionRequest*>(observable);
  assert(request->token->node);
  auto recipientHeader = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front()->getRecipientHeader(request->token->status);
  messageDeliveryRequests.emplace_back(request->token->weak_from_this(), request->weak_from_this(), recipientHeader);
}

std::shared_ptr<Event> FirstMatchingMessageDelivery::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr, recipientHeader ] : messageDeliveryRequests ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      if ( request_ptr.lock() )  {
        for ( auto candidate : token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates ) {
          if ( auto it = systemState->outbox.find(candidate); it != systemState->outbox.end() ) {
            for ( auto& [message_ptr] : it->second ) {
              auto message = message_ptr.lock();
              if ( message && message->matches(recipientHeader) ) {
                return std::make_unique<MessageDeliveryDecision>(token.get(), message.get(), recipientHeader);
              }
            }
          }
        }
      }
    }
  }
  return nullptr;
}

