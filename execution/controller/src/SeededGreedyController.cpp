#include "SeededGreedyController.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <iostream>

using namespace BPMNOS::Execution;

SeededGreedyController::SeededGreedyController(const BPMNOS::Model::Scenario* scenario, Evaluator* evaluator)
  : CPController(scenario)
  , evaluator(evaluator)
  , _seed( CPSeed(this,CPSeed::defaultSeed(getVertices().size())) )
{
  choiceDispatcher = std::make_unique<BestLimitedChoice>(evaluator);
}

bool SeededGreedyController::setSeed(const std::list<size_t>& seed) {
  _seed = CPSeed(this,seed);
  return ( _seed.coverage() == 1.0 );
}

CP::Solution& SeededGreedyController::createSolution() {
  auto& solution = CPController::createSolution();
  auto& sequence = model.getSequences().front();
  auto sequenceVariableValues = _seed.getSequence();

  if( sequence.variables.size() != sequenceVariableValues.size() ) {
    throw std::runtime_error("SeededGreedyController: illegal seed");
  }

  std::vector<size_t> positions(sequenceVariableValues.size());
  for ( size_t index = 0; index < sequenceVariableValues.size(); index++ ) {
    positions[ sequenceVariableValues[index]-1 ] = index + 1;
  }
  solution.setSequenceValues( sequence, positions );
  createDecisionQueue();
  
  return solution;
}

void SeededGreedyController::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Execution::Observable::Type::Message
  );
  
  CPController::connect(mediator);
}

void SeededGreedyController::notice(const Observable* observable) {
  CPController::notice(observable);
  if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
    if ( message->state == Message::State::CREATED ) {
      messages.emplace_back( message->weak_from_this() );
    }
  }
}

std::shared_ptr<Event> SeededGreedyController::createEntryEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  // instant entry
  setTimestamp(vertex,systemState->getTime());
  return CPController::createEntryEvent(systemState, token, vertex);
}

std::shared_ptr<Event> SeededGreedyController::createExitEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  // instant exit
  setTimestamp(vertex,systemState->getTime());
  return CPController::createExitEvent(systemState, token, vertex);
}

std::shared_ptr<Event> SeededGreedyController::createChoiceEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  // instant choice
  setTimestamp(vertex,systemState->getTime());

  auto best = choiceDispatcher->determineBestChoices(token->decisionRequest);
  if (!best) {
    // no feasible choices found
    return std::make_shared<ErrorEvent>(token);
  }
  
  return best;
}

std::shared_ptr<Event> SeededGreedyController::createMessageDeliveryEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  // instant message delivery
  setTimestamp(vertex,systemState->getTime());
  
  // obtain message candidates
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(token->status);
  auto recipientHeader = messageDefinition->getRecipientHeader(token->getAttributeRegistry(),token->status,*token->data,token->globals);
  auto senderCandidates = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  std::list< std::shared_ptr<const Message> > candidates;
  // determine candidate messages
  for ( auto& [ message_ptr ] : messages ) {
    if( auto message = message_ptr.lock();
      message &&
      std::ranges::contains(senderCandidates, message->origin) &&
      message->matches(recipientHeader)
    ) {
      candidates.emplace_back( message );
    }
  }

  // evaluate message candidates
  std::shared_ptr<MessageDeliveryDecision> best = nullptr;
  for ( auto& message : candidates ) {
    auto decision = std::make_shared<MessageDeliveryDecision>(token, message.get(), evaluator);
    decision->evaluate();
    if ( 
      decision->evaluation.has_value() &&
      ( !best || best->evaluation.value() < decision->evaluation.value() )
    ) {
      best = decision;
    }
  }
  
  if (!best) {
    // no message can be delivered
    return std::make_shared<ErrorEvent>(token);
  }
  
  auto message = best->message.lock();
  assert( message );
  auto senderId = message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value();
  auto& [senderEntry,senderExit] = flattenedGraph.vertexMap.at({senderId,{},message->origin}); // TODO: sender must not be within loop
  setMessageFlowVariableValue(&senderEntry,vertex);
/*
  auto& siblings = flattenedGraph.vertexMap.at(message->origin).at(message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value());


  assert( flattenedGraph.vertexMap.contains(message->origin) );
  assert( flattenedGraph.vertexMap.at(message->origin).contains(message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value()) );
  auto& siblings = flattenedGraph.vertexMap.at(message->origin).at(message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value());
  auto senderEntry = &siblings.front().get(); // TODO: Multi-instance/loop
//  auto sender = vertexMap.at( std::make_tuple(message->origin, message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value(), Vertex::Type::ENTRY) );
  setMessageFlowVariableValue(senderEntry,vertex);
*/
  
  return best;
}

