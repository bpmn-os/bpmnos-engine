#include "MessageDeliveries.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DecisionRequest.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>
#include <ranges>

using namespace BPMNOS::Execution;

MessageDeliveries::MessageDeliveries(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

void MessageDeliveries::connect(Notifier* notifier) {
  notifier->addSubscriber(this,
    Observable::Type::MessageDeliveryRequest,
    Observable::Type::Message,
    Observable::Type::DataUpdate,
    Observable::Type::SystemState
  );
}

void MessageDeliveries::clear() {
  CachedCandidates::clear();   // the cached decisions and their weak indexes
  requests.clear();
  messages.clear();
}

void MessageDeliveries::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);

    auto messageDefinition = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(request->token->status);
    auto recipientHeader = messageDefinition->getRecipientHeader(request->token->getAttributeRegistry(),request->token->status,*request->token->data,request->token->globals);
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
        addDecision( request->token->weak_from_this(), request->weak_from_this(), message_ptr, decision );
      }
    }
  }
  else if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
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
          addDecision( token_ptr, request_ptr, message->weak_from_this(), decision );
        }
      }
    }
  }
  else if ( observable->getObservableType() == Observable::Type::SystemState ) {
    clear();   // start from a clean cache, then rebuild from the installed state
    systemState = static_cast<const SystemState*>(observable);
    // rebuild the message pool first, then replay the pending delivery requests so they match against it
    for ( auto& message : systemState->messages ) {
      notice( message.get() );
    }
    for ( auto& [_, request_ptr] : systemState->pendingMessageDeliveryDecisions ) {
      if ( auto request = request_ptr.lock() ) {
        notice( request.get() );
      }
    }
  }
  else {
    CachedCandidates::notice(observable);
  }
}

void MessageDeliveries::evaluateCandidates() {
  evaluateDecisions(
    [this]( std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::weak_ptr<const Message> message_ptr, std::shared_ptr<Decision> decision ) -> std::shared_ptr<Event> {
      assert(decision);
      decision->evaluate();
      addEvaluation( token_ptr, request_ptr, message_ptr, std::move(decision) );
      return nullptr;   // evaluate all candidates; the dispatcher takes the best feasible
    }
  );
}
