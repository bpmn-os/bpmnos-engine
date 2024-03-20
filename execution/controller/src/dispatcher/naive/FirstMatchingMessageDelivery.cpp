#include "FirstMatchingMessageDelivery.h"
#include "execution/engine/src/Mediator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"
#include "model/parser/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

FirstMatchingMessageDelivery::FirstMatchingMessageDelivery()
{
}

void FirstMatchingMessageDelivery::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::MessageDeliveryRequest,
    Execution::Observable::Type::Message
  );
  EventDispatcher::connect(mediator);
}

void FirstMatchingMessageDelivery::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);
    auto recipientHeader = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front()->getRecipientHeader(request->token->status);
    auto senderCandidates = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
    auto_list< std::weak_ptr<const Message> > candidates;
    // determine candidate messages
    for ( auto& [ message_ptr ] : messages ) {
      if( auto message = message_ptr.lock();
        message &&
        std::ranges::contains(senderCandidates, message->origin) &&
        message->matches(recipientHeader)
      ) {
        candidates.emplace_back( message->weak_from_this() );
      }
    }
    messageDeliveryRequests.emplace_back(request->token->weak_from_this(), request->weak_from_this(), candidates, recipientHeader);
  }
  else if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
    if ( message->state == Message::State::CREATED ) {
      messages.emplace_back( message->weak_from_this() );
      // add new message to relevant candidate lists
      auto recipientCandidates = message->origin->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
      for ( auto& [token_ptr, request_ptr, candidates, recipientHeader ] : messageDeliveryRequests ) {
        if ( auto token = token_ptr.lock();
          token &&
          std::ranges::contains(recipientCandidates, token->node) &&
          message->matches(recipientHeader)
        ) {
          candidates.emplace_back( message->weak_from_this() );
        }
      }
    }
  }
  else {
    assert(!"Unexpected observable type");
  }
}

std::shared_ptr<Event> FirstMatchingMessageDelivery::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr, candidates, recipientHeader ] : messageDeliveryRequests ) {
    if( auto token = token_ptr.lock() )  {
      if ( request_ptr.lock() )  {
        for ( auto& [message_ptr] : candidates ) {
          if ( auto message = message_ptr.lock() ) {
            return std::make_shared<MessageDeliveryDecision>(token.get(), message.get());
          }
        }
      }
    }
  }
  return nullptr;
}

