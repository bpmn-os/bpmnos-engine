#include "SeededGreedyController.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <iostream>

using namespace BPMNOS::Execution;

SeededGreedyController::SeededGreedyController(const BPMNOS::Execution::FlattenedGraph* flattenedGraph, Evaluator* evaluator)
  : SeededController(flattenedGraph)
  , evaluator(evaluator)
{
  choiceDispatcher = std::make_unique<BestEnumeratedChoice>(evaluator);
}

void SeededGreedyController::notice(const Observable* observable) {
//std::cerr << "SeededGreedyController::notice" << std::endl;
  SeededController::notice(observable);
  if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
    if ( message->state == Message::State::CREATED ) {
//std::cerr << "Message created: " << message->jsonify() << std::endl;
      messages.emplace_back( message->weak_from_this() );
    }
  }
//std::cerr << "SeededGreedyController::noticed" << std::endl;
}

std::shared_ptr<Event> SeededGreedyController::createEntryEvent([[maybe_unused]] const SystemState* systemState, const Token* token, [[maybe_unused]] const Vertex* vertex) {
  // instant entry ( if feasible )
  auto decision = std::make_shared<EntryDecision>(token, evaluator);
  auto reward = decision->evaluate();
  if (  reward.has_value() ) {
    return decision;
  }
  return nullptr;
}

std::shared_ptr<Event> SeededGreedyController::createExitEvent([[maybe_unused]] const SystemState* systemState, const Token* token, [[maybe_unused]] const Vertex* vertex) {
  // instant exit ( if feasible )
  auto decision = std::make_shared<ExitDecision>(token, evaluator);
  auto reward = decision->evaluate();
  if ( reward.has_value() ) {
    return decision;
  }
  return nullptr;
}

std::shared_ptr<Event> SeededGreedyController::createChoiceEvent([[maybe_unused]] const SystemState* systemState, const Token* token, [[maybe_unused]] const Vertex* vertex) {
  // instant choice
  auto best = choiceDispatcher->determineBestChoices(token->decisionRequest);
  if (!best) {
    // no feasible choices found
    return nullptr;
    // return std::make_shared<ErrorEvent>(token);
  }

  return best;
}

std::shared_ptr<Event> SeededGreedyController::createMessageDeliveryEvent([[maybe_unused]] const SystemState* systemState, const Token* token, [[maybe_unused]] const Vertex* vertex) {
//std::cerr << "SeededGreedyController::createMessageDeliveryEvent" << std::endl;
  // obtain message candidates
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(token->status);
  auto recipientHeader = messageDefinition->getRecipientHeader(token->getAttributeRegistry(),token->status,*token->data,token->globals);
  auto senderCandidates = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  std::list< std::shared_ptr<const Message> > candidates;
//std::cerr << "Messages: " << !messages.empty() << std::endl;
//std::cerr << "senderCandidate: " << senderCandidates.front()->id << std::endl;
  // determine candidate messages
  for ( auto& [ message_ptr ] : messages ) {
//std::cerr << "Message: " << message_ptr.lock()->jsonify() << "/" << message_ptr.lock()->origin->id << "/" << message_ptr.lock()->matches(recipientHeader) << std::endl;
    if( auto message = message_ptr.lock();
      message &&
      std::ranges::contains(senderCandidates, message->origin) &&
      message->matches(recipientHeader)
    ) {
//std::cerr << "Candidate: " << message->jsonify() << std::endl;
      candidates.emplace_back( message );
    }
  }

  // evaluate message candidates
  std::shared_ptr<MessageDeliveryDecision> best = nullptr;
  for ( auto& message : candidates ) {
    auto decision = std::make_shared<MessageDeliveryDecision>(token, message.get(), evaluator);
    auto reward = decision->evaluate();
    if ( 
      reward.has_value() &&
      ( !best || best->reward.value() < reward.value() )
    ) {
      best = decision;
    }
  }
  
  if (!best) {
    // no message can be delivered
    if ( token->node->represents<BPMN::MessageStartEvent>() ) {
      assert( vertex->exit<BPMN::MessageStartEvent>() );
//std::cerr << "MessageStartEvent is not triggered: " << vertex->shortReference() << std::endl;
    }    
    return nullptr;
//    return std::make_shared<ErrorEvent>(token);
  }
  
  return best;
}

