#include "BestMatchingMessageDelivery.h"
#include "execution/engine/src/Mediator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestMatchingMessageDelivery::BestMatchingMessageDelivery( std::function<std::optional<double>(const Event* event)> evaluator )
  : evaluator(evaluator)
{
}

void BestMatchingMessageDelivery::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::MessageDeliveryRequest,
    Execution::Observable::Type::Message
  );
  EventDispatcher::connect(mediator);
}

void BestMatchingMessageDelivery::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);
//std::cerr << "Request: " << request->token->jsonify().dump() << std::endl;
    
    auto recipientHeader = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front()->getRecipientHeader(request->token->status);
    requests.emplace_back( request->token->weak_from_this(), request->weak_from_this(), recipientHeader );

    auto senderCandidates = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
    // determine candidate decisions
    for ( auto& [ message_ptr ] : messages ) {
      if ( auto message = message_ptr.lock();
        message &&
        std::ranges::contains(senderCandidates, message->origin) &&
        message->matches(recipientHeader)
      ) {
        auto decision = std::make_shared<MessageDeliveryDecision>(request->token, message.get(), evaluator);
        decisions.emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), request->token->weak_from_this(), request->weak_from_this(), message_ptr, decision );
      }
    }
  }
  else if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
//std::cerr << "Message: " << message->jsonify().dump() << std::endl;
    if ( message->state == Message::State::CREATED ) {
      messages.emplace_back( message->weak_from_this() );
      // add new decision
      auto recipientCandidates = message->origin->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
      for ( auto& [token_ptr, request_ptr, recipientHeader ] : requests ) {
        if ( auto token = token_ptr.lock();
          token &&
          std::ranges::contains(recipientCandidates, token->node) &&
          message->matches(recipientHeader)
        ) {
          auto decision = std::make_shared<MessageDeliveryDecision>(token.get(), message, evaluator);
          decisions.emplace( decision->evaluation.value_or( std::numeric_limits<double>::max() ), token_ptr, request_ptr, message->weak_from_this(), decision );
        }
      }
    }
  }
  else {
    assert(!"Unexpected observable type");
  }
}

std::shared_ptr<Event> BestMatchingMessageDelivery::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto [evaluation, token_ptr, request_ptr, message_ptr, decision ] : decisions ) {
//std::cerr << "Dispatch: " << evaluation << std::endl;
    return decision;
  }
  return nullptr;
}

