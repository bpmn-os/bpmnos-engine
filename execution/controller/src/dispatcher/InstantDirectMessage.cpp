#include "InstantDirectMessage.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

InstantDirectMessage::InstantDirectMessage()
{
}

void InstantDirectMessage::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Execution::Observable::Type::MessageDeliveryRequest,
    Execution::Observable::Type::Message,
    Execution::Observable::Type::SystemState
  );
  EventDispatcher::connect(mediator);
}

void InstantDirectMessage::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);
    auto messageDefinition = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(request->token->status);
    auto recipientHeader = messageDefinition->getRecipientHeader(request->token->getAttributeRegistry(),request->token->status,*request->token->data,request->token->globals);
    auto senderCandidates = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
    auto_list< std::weak_ptr<const Message> > candidates;
    // determine explicitly addressed candidate messages
    for ( auto& [ message_ptr ] : messages ) {
      if( auto message = message_ptr.lock();
        message &&
        std::ranges::contains(senderCandidates, message->origin) &&
        message->matches(recipientHeader) &&
        ( message->recipient.has_value() || recipientHeader[BPMNOS::Model::MessageDefinition::Index::Sender].has_value() )
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
          message->matches(recipientHeader) &&
          ( message->recipient.has_value() || recipientHeader[BPMNOS::Model::MessageDefinition::Index::Sender].has_value() )
        ) {
          candidates.emplace_back( message->weak_from_this() );
        }
      }
    }
  }
  else if ( observable->getObservableType() == Observable::Type::SystemState ) {
    // A foreign state was installed (e.g. on resume). Rebuild from it: incremental Message/MessageDeliveryRequest
    // notices are not replayed for state that already existed at install time, so a directly-addressed delivery
    // pending at install (e.g. an order waiting at NoticeJobCompletionTask for its completion) would otherwise be
    // invisible here. Replay the created messages first, then the pending delivery requests so they match against
    // them — mirroring MessageDeliveries::notice(SystemState).
    auto systemState = static_cast<const SystemState*>(observable);
    messages.clear();
    messageDeliveryRequests.clear();
    for ( auto& message : systemState->messages ) {
      notice( message.get() );
    }
    for ( auto& [token_ptr, request_ptr] : systemState->pendingMessageDeliveryDecisions ) {
      if ( auto request = request_ptr.lock() ) {
        notice( request.get() );
      }
    }
  }
  else {
    assert(false && "Unexpected observable type");
  }
}

std::shared_ptr<Event> InstantDirectMessage::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [token_ptr, request_ptr, candidates, recipientHeader ] : messageDeliveryRequests ) {
    if( auto token = token_ptr.lock() )  {
      if ( request_ptr.lock() )  {
        for ( auto& [message_ptr] : candidates ) {
          if ( auto message = message_ptr.lock() ) {
            return std::make_shared<MessageDeliveryEvent>(token.get(), message.get());
          }
        }
      }
    }
  }
  return nullptr;
}
