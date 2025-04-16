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
std::cerr << "updated seed: ";
for ( auto i : _seed.getSeed() ) std::cerr << i << ", ";
std::cerr << std::endl;

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
  initializeEventQueue();
  
  return solution;
}

void SeededGreedyController::notice(const Observable* observable) {
  CPController::notice(observable);
  if ( observable->getObservableType() == Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
    if ( message->state == Message::State::CREATED ) {
//std::cerr << "Message created: " << message->jsonify() << std::endl;
      messages.emplace_back( message->weak_from_this() );
    }
  }
}

std::shared_ptr<Event> SeededGreedyController::createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  // instant entry
  auto decision = std::make_shared<EntryDecision>(token, evaluator);
  decision->evaluate();
  if (  decision->evaluation.has_value() ) {
    setTimestamp(vertex,systemState->getTime());
    return decision;
  }
  return nullptr;
}

std::shared_ptr<Event> SeededGreedyController::createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  // instant exit
  auto decision = std::make_shared<ExitDecision>(token, evaluator);
  decision->evaluate();
  if (  decision->evaluation.has_value() ) {
    setTimestamp(vertex,systemState->getTime());
    return decision;
  }
  return nullptr;
}

std::shared_ptr<Event> SeededGreedyController::createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  // set timestamp of choice
  setLocalAttributeValue(vertex,BPMNOS::Model::ExtensionElements::Index::Timestamp,systemState->getTime());

  auto best = choiceDispatcher->determineBestChoices(token->decisionRequest);
  if (!best) {
    // no feasible choices found
    return nullptr;
    // return std::make_shared<ErrorEvent>(token);
  }
  
  // apply choices 
  auto decision = dynamic_cast<ChoiceDecision*>(best.get());
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements );
  assert( extensionElements->choices.size() == decision->choices.size() );
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    assert( decision->choices[i].has_value() );
    setLocalAttributeValue(vertex,extensionElements->choices[i]->attribute->index,decision->choices[i].value());
  }
  
  return best;
}

std::shared_ptr<Event> SeededGreedyController::createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
std::cerr << "SeededGreedyController::createMessageDeliveryEvent" << std::endl;
  // instant message delivery
  setTimestamp(vertex,systemState->getTime());
  
  // obtain message candidates
  auto messageDefinition = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getMessageDefinition(token->status);
  auto recipientHeader = messageDefinition->getRecipientHeader(token->getAttributeRegistry(),token->status,*token->data,token->globals);
  auto senderCandidates = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  std::list< std::shared_ptr<const Message> > candidates;
std::cerr << "Messages: " << !messages.empty() << std::endl;
std::cerr << "senderCandidate: " << senderCandidates.front()->id << std::endl;
  // determine candidate messages
  for ( auto& [ message_ptr ] : messages ) {
std::cerr << "Message: " << message_ptr.lock()->jsonify() << "/" << message_ptr.lock()->origin->id << "/" << message_ptr.lock()->matches(recipientHeader) << std::endl;
    if( auto message = message_ptr.lock();
      message &&
      std::ranges::contains(senderCandidates, message->origin) &&
      message->matches(recipientHeader)
    ) {
std::cerr << "Candidate: " << message->jsonify() << std::endl;
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
    if ( token->node->represents<BPMN::MessageStartEvent>() ) {
      assert( vertex->exit<BPMN::MessageStartEvent>() );
      unvisited(vertex); 
std::cerr << "MessageStartEvent is not triggered: " << vertex->shortReference() << std::endl;
    }    
    return nullptr;
//    return std::make_shared<ErrorEvent>(token);
  }
  
  auto message = best->message.lock();
  assert( message );
  auto senderId = message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value();
  assert( flattenedGraph.vertexMap.contains({senderId,{},message->origin}) );
  auto& [senderEntry,senderExit] = flattenedGraph.vertexMap.at({senderId,{},message->origin}); // TODO: sender must not be within loop
  setMessageDeliveryVariableValues(&senderEntry,vertex,systemState->getTime());
  
  return best;
}

