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
  for ( auto& [ token_ptr, request_ptr, message_ptr, decision ] : decisionsWithoutEvaluation ) {
    assert(decision);
    if ( decision ) {
//std::cerr << "Re-evaluate message delivery decision: " << decision->jsonify().dump() << std::endl;
      evaluate( token_ptr, request_ptr, message_ptr, std::move(decision) );
    }
  }
  decisionsWithoutEvaluation.clear();


/*
  for ( auto [ cost, token_ptr, request_ptr, message_ptr, event_ptr ] : evaluatedDecisions ) {
if ( cost < 1000 )
std::cerr << "Message delivery decision " << event_ptr.lock()->jsonify() << " evaluated with " << cost << std::endl;
  }
*/
  for ( auto [ cost, token_ptr, request_ptr, message_ptr, event_ptr ] : evaluatedDecisions ) {
    // return best evaluated decision
//std::cerr << "\nBest message delivery decision " << event_ptr.lock()->jsonify() << " evaluated with " << cost << std::endl;
    return event_ptr.lock();
  }

//std::cerr << "No evaluated message delivery decision" << std::endl;
  return nullptr;
}

void BestMatchingMessageDelivery::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::MessageDeliveryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    assert(request->token->node);
//std::cerr << "MessageDeliveryRequest: " << request->token->jsonify().dump() << std::endl;
    
    auto recipientHeader = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front()->getRecipientHeader(request->token->getAttributeRegistry(),request->token->status,*request->token->data,request->token->globals);
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
        decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), message_ptr, decision );
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
          decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, message->weak_from_this(), decision );
        }
      }
    }
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

