#include "BestMatchingMessageDelivery.h"
#include "execution/engine/src/Mediator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

BestMatchingMessageDelivery::BestMatchingMessageDelivery(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
{
}

void BestMatchingMessageDelivery::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::MessageDeliveryRequest,
    Execution::Observable::Type::Message
  );
  GreedyDispatcher::connect(mediator);
}

std::shared_ptr<Event> BestMatchingMessageDelivery::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [ token_ptr, request_ptr, message_ptr, decision ] : decisionStore.decisionsWithoutEvaluation ) {
    assert(decision);
    if ( decision ) {
      decision->evaluate();
//std::cerr << "Re-evaluated message delivery decision: " << decision << decision->jsonify().dump() << std::endl;
      decisionStore.addEvaluation( token_ptr, request_ptr, message_ptr, std::move(decision) );
    }
  }
  decisionStore.decisionsWithoutEvaluation.clear();

  // return best evaluated decision
  return decisionStore.getBestDecision();
}

void BestMatchingMessageDelivery::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);
//std::cerr << "MessageDeliveryRequest: " << request->token->jsonify().dump() << std::endl;
    
    auto messageDefinition = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(request->token->status);
    auto recipientHeader = messageDefinition->getRecipientHeader(request->token->getAttributeRegistry(),request->token->status,*request->token->data,request->token->globals);
    requests.emplace_back( request->token->weak_from_this(), request->weak_from_this(), recipientHeader );

    auto senderCandidates = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
    // determine candidate decisions
    for ( auto& [ message_ptr ] : messages ) {
//std::cerr << "Candidate: " <<std::make_shared<MessageDeliveryDecision>(request->token, message_ptr.lock().get(), evaluator)->jsonify().dump() << std::endl;
      if ( auto message = message_ptr.lock();
        message &&
        std::ranges::contains(senderCandidates, message->origin) &&
        message->matches(recipientHeader)
      ) {
//std::cerr << "match" << std::endl;
        auto decision = std::make_shared<MessageDeliveryDecision>(request->token, message.get(), evaluator);
        decisionStore.decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), message_ptr, decision );
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
//std::cerr << "Candidate: " <<std::make_shared<MessageDeliveryDecision>(token_ptr.lock().get(), message, evaluator)->jsonify().dump() << std::endl;
        if ( auto token = token_ptr.lock();
          token &&
          std::ranges::contains(recipientCandidates, token->node) &&
          message->matches(recipientHeader)
        ) {
//std::cerr << "match" << std::endl;
          auto decision = std::make_shared<MessageDeliveryDecision>(token.get(), message, evaluator);
          decisionStore.decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, message->weak_from_this(), decision );
        }
      }
    }
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

