#include "SeededController.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"
#include <iostream>

using namespace BPMNOS::Execution;

SeededController::SeededController(const BPMNOS::Model::Scenario* scenario, Config config)
 : scenario(scenario)
 , config(std::move(config))
 , flattenedGraph(FlattenedGraph(scenario))
 , model(flattenedGraph)
{
  // initialize seed
  seed.resize( flattenedGraph.vertices.size() );
  std::iota(seed.begin(), seed.end(), 1);

  createSolution();

  // initialize seed
  seed.resize( flattenedGraph.vertices.size() );
  std::iota(seed.begin(), seed.end(), 1);
  lastPosition = 0;
  initializePendingVertices();
}

void SeededController::connect(Mediator* mediator) {
  readyHandler.connect(mediator);
  completionHandler.connect(mediator);
  Controller::connect(mediator);
}

void SeededController::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Execution::Observable::Type::Token
  );
}

bool SeededController::setSeed(const std::list<size_t> initialSeed) {
  lastPosition = 0;
  seed = std::move(initialSeed);
  if ( seed.size() < flattenedGraph.vertices.size() ) {
    return false;
  }
std::cerr << "updated seed: ";
for ( auto i : seed ) std::cerr << i << ", ";
std::cerr << std::endl;
  initializePendingVertices();

  return true;
}

std::list< const SeededController::Vertex* >::iterator SeededController::finalizeVertexPosition(const Vertex* vertex) {
//  assert( std::ranges::contains(pendingVertices,vertex) );
//std::cerr << "finalizeVertexPosition " << vertex->reference() << std::endl;
  auto it = std::find(pendingVertices.begin(), pendingVertices.end(), vertex);
  if( it != pendingVertices.end() ) {
//std::cerr << "Removed " << vertex->reference() << " from " << pendingVertices.size() << std::endl;
    it = pendingVertices.erase(it);
    processedVertices.push_back(vertex);
    // change position of vertex
//TODO    _solution->setPosition( vertex, ++lastPosition);
//std::cerr << "position(" << vertex->reference() << ") = " << lastPosition << std::endl;
//std::cerr << "-> position(" << vertex->reference() << "/" << (vertex->index+1) << ") = " << lastPosition << std::endl;
//std::cerr << "visit(" << vertex->shortReference() << ") = " << _solution->evaluate( visit.at(vertex) ).value_or(-1) << std::endl;
  }
  return it;
  
};

void SeededController::fetchPendingPredecessors(std::unordered_set<const Vertex*>& predecessors, const Vertex* vertex) const {
  for ( auto& [_,predecessor] : vertex->inflows ) {
//std::cerr << predecessor.reference() << "/" << predecessors.contains(&predecessor) << "/" << std::ranges::contains(pendingVertices,&predecessor) << std::endl;
    if ( !predecessors.contains(predecessor) && std::ranges::contains(pendingVertices,predecessor) ) {
//std::cerr << "Added " << predecessor.reference() << std::endl;
      predecessors.emplace(predecessor);
      fetchPendingPredecessors(predecessors,predecessor);
    }
  }
  for ( auto predecessor : vertex->predecessors ) {
    if ( !predecessors.contains(predecessor) && std::ranges::contains(pendingVertices,predecessor) ) {
//std::cerr << "Added " << predecessor.reference() << std::endl;
      predecessors.emplace(predecessor);
      fetchPendingPredecessors(predecessors,predecessor);
    }
  }
}

void SeededController::finalizePredecessorPositions(const Vertex* vertex) {
  std::unordered_set<const Vertex*> pendingPredecessors;
  fetchPendingPredecessors(pendingPredecessors,vertex);
//std::cerr << "finalize " << pendingPredecessors.size() << " predecessor positions: " << vertex->reference() << std::endl;

  auto it = pendingPredecessors.begin();
  while ( it != pendingPredecessors.end() ) {
    const Vertex* predecessor = *it;
    if ( !std::ranges::contains(pendingVertices,predecessor) ) {
      pendingPredecessors.erase(it);
      it = pendingPredecessors.begin();
      continue;
    }
//std::cerr << "other: " << predecessor->reference() << std::endl;
    if ( !hasPendingPredecessor(predecessor) ) {
      if ( flattenedGraph.dummies.contains(predecessor) ) {
        assert( std::ranges::contains(pendingVertices,predecessor) );
        finalizeVertexPosition( predecessor );
      }
      else if ( predecessor->type == Vertex::Type::ENTRY ) {
        assert( std::ranges::contains(pendingVertices,predecessor) );
        finalizeUnvisited( predecessor );
      }
      pendingPredecessors.erase(it);
      it = pendingPredecessors.begin();
    }
    else {
      it++;
    }
  }

//std::cerr << "finalizedPredecessorPositions" << vertex->reference() << std::endl;
}

void SeededController::finalizeUnvisitedChildren(const Vertex* vertex) {
  assert( vertex == entry(vertex) );
  std::list<const Vertex*> successors;
  for ( auto successor : vertex->successors ) {
    if ( successor == entry(successor) ) {
      successors.push_back(successor);
    }
  }
  auto it = successors.begin();
  while ( it != successors.end() ) {
    assert( std::ranges::contains(pendingVertices,*it) );
    if ( !hasPendingPredecessor(*it) ) {
      finalizeUnvisited( *it );
      successors.erase(it);
      it = successors.begin();
    }
    else {
      it++;
    }
  }
}

std::list< const SeededController::Vertex* >::iterator SeededController::finalizeUnvisited(const Vertex* vertex) {
  assert( vertex == entry(vertex) );
  
  auto it = finalizeVertexPosition(vertex);
//TODO  _solution->unvisitEntry(vertex);

  auto finalizeExit = [&]() {
    if (vertex->node->represents<BPMN::Scope>()) {
      finalizeUnvisitedChildren(entry(vertex));
    }
    finalizeVertexPosition(exit(vertex));
  };

  if ( it == pendingVertices.begin() ) { 
    finalizeExit();
    it = pendingVertices.begin(); // may have changed
  }
  else {
    it--;
    finalizeExit();
    it++;
  }


//TODO  _solution->unvisitExit(exit(vertex));

  return it;
}


void SeededController::synchronizeSolution(const Token* token) {
  if ( terminationEvent ) {
    return;
  }

//std::cerr << "Finalize position(s): " << token->jsonify() << std::endl;    

  if ( token->state == Token::State::FAILED ) {
    terminationEvent = std::make_shared<TerminationEvent>();
  }

  if ( token->node && token->state == Token::State::WITHDRAWN && token->node->represents<BPMN::TypedStartEvent>() ) {
    if ( auto vertex = flattenedGraph.getVertex(token) ) {
//std::cerr << "clear withdrawn start event: " << vertex->reference() << std::endl;
      finalizeUnvisited(entry(vertex));
    }
  }

  if ( token->state == Token::State::ENTERED ) {
    if ( auto vertex = flattenedGraph.getVertex(token) ) {
      if ( vertex->entry<BPMN::TypedStartEvent>() ) {
//std::cerr << "ignore typed start event entry: " << token->jsonify() << std::endl;
        return;
      }
      finalizePredecessorPositions(vertex);
      finalizeVertexPosition(vertex);
      if ( 
        vertex->entry<BPMN::UntypedStartEvent>() || 
        vertex->entry<BPMN::Gateway>() || 
        ( vertex->entry<BPMN::ThrowEvent>() && !vertex->entry<BPMN::SendTask>() ) 
      ) {
//std::cerr << "clear instantaneous exit: " << token->jsonify() << std::endl;
        finalizePredecessorPositions(exit(vertex));
        finalizeVertexPosition(exit(vertex));
      }
    }
  }
  else if ( token->node && token->state == Token::State::COMPLETED && token->node->represents<BPMN::TypedStartEvent>() ) {
    auto vertex = flattenedGraph.getVertex(token);
    assert( vertex );
    assert( vertex->exit<BPMN::TypedStartEvent>() );
    finalizePredecessorPositions(entry(vertex));
    finalizeVertexPosition(entry(vertex));
    finalizeVertexPosition(vertex);
  }
  else if ( 
    ( !token->node && token->state == Token::State::COMPLETED ) ||
    ( token->node && token->node->represents<BPMN::Scope>() && token->state == Token::State::COMPLETED )
  ) {
//std::cerr << "Scope completed" << std::endl;
    // Scope is completed, all pending predecessors are not visited
    auto vertex = flattenedGraph.getVertex(token);
    assert( vertex );
    assert( vertex == exit(vertex) );
    finalizePredecessorPositions(vertex);    
  }
  else if ( 
    ( !token->node && token->state == Token::State::DONE ) || // Process
    ( token->node && token->state == Token::State::EXITING && !token->node->represents<BPMN::TypedStartEvent>() ) || // Activity
    ( token->node && token->state == Token::State::COMPLETED && token->node->represents<BPMN::CatchEvent>() && !token->node->represents<BPMN::ReceiveTask>() )
  ) { 
    if ( auto vertex = flattenedGraph.getVertex(token) ) {
      auto entryVertex = entry(vertex);
      if ( entryVertex->inflows.size() == 1 && entryVertex->inflows.front().second->node->represents<BPMN::EventBasedGateway>() ) {
        assert( vertex->exit<BPMN::CatchEvent>() );
        auto gateway = entryVertex->inflows.front().second;
        for ( auto& [ _, target ] : gateway->outflows ) {
          if ( target != entryVertex ) {
            finalizeUnvisited( target );
          }
        } 
      }
      finalizePredecessorPositions(vertex);
      finalizeVertexPosition(vertex);
//std::cerr << "cleared exit event: " << vertex->reference() << std::endl;
    }
  }
//std::cerr << "check state" << std::endl;    

  if ( !token->node && token->state != Token::State::ENTERED && token->state != Token::State::DONE ) {
    // token at process, but with irrelevant state
    return;
  }
  if ( token->node && token->state != Token::State::ENTERED && token->state != Token::State::EXITING ) {
    // token at flow node, but with irrelevant state
    return;
  }
  if ( token->node && token->state != Token::State::EXITING && token->node->represents<BPMN::TypedStartEvent>() ) {
    // token at typed start event, but not triggered
    return;
  }
}

void SeededController::notice(const Observable* observable) {
//std::cerr << "notice" << std::endl;
  Controller::notice(observable);
  
  if( observable->getObservableType() ==  Execution::Observable::Type::Token ) {
//std::cerr << "synchronizeSolution" << std::endl;
    synchronizeSolution( static_cast<const Token*>(observable) );
//std::cerr << "synchronizedSolution" << std::endl;
  }
  else if( observable->getObservableType() ==  Execution::Observable::Type::SequentialPerformerUpdate ) {
    auto performerToken = static_cast<const SequentialPerformerUpdate*>(observable)->token;
//std::cerr << "SequentialPerformerUpdate: " << performerToken->jsonify() << (performerToken->performing ? " is busy" : " is idle") << std::endl;
    if ( performerToken->performing ) {
      performing[ entry(flattenedGraph.getVertex(performerToken)) ] = entry(flattenedGraph.getVertex(performerToken->performing));
    }
    else {
      performing[ entry(flattenedGraph.getVertex(performerToken)) ] = nullptr;
    }
  }
//std::cerr << "noticed" << std::endl;
}

bool SeededController::hasPendingPredecessor(const Vertex* vertex) const {
  if ( vertex == pendingVertices.front() ) {
    return false;
  }
  for ( auto& [_,predecessor] : vertex->inflows ) {
    if ( vertex == exit(predecessor) && vertex->exit<BPMN::TypedStartEvent>()  ) {
      continue;
    }
    if ( !std::ranges::contains(processedVertices,predecessor) ) {
//std::cerr << predecessor.reference() << " precedes " << vertex->reference() << std::endl;
      return true;
    }
  }
  for ( auto predecessor : vertex->predecessors ) {
    if ( !std::ranges::contains(processedVertices,predecessor) ) {
//std::cerr << predecessor.reference() << " precedes " << vertex->reference() << std::endl;
      return true;
    }
  }
  return false;
}

std::list< const SeededController::Vertex* >::iterator SeededController::finalizeUnvisitedTypedStartEvents(std::list< const Vertex* >::iterator it) {
  auto node = (*it)->node;
  auto parentEntry = (*it)->parent.value().first;

  auto it2 = it;
  // find first iterator to vertex at other node
  it = std::find_if_not(it, pendingVertices.end(), [&](auto other) { 
    return (node == other->node && parentEntry == other->parent.value().first );
  });

  // finalize message start events that are not visited
  while ( it2 != pendingVertices.end() ) {
    if ( (*it2)->type == Vertex::Type::EXIT && node == (*it2)->node && parentEntry == (*it2)->parent.value().first ) {
//std::cerr << "unvisit: " << (*it2)->shortReference() << std::endl;
      it2 = finalizeUnvisited(entry(*it2));
    }
    else {
      it2++;
    }
  }
  return it;
}

bool SeededController::hasPendingRecipient(const Vertex* vertex) const {
  assert( vertex->exit<BPMN::SendTask>() );
  for ( auto recipient : entry(vertex)->recipients ) {
    if ( std::ranges::contains(pendingVertices,recipient) ) {
      return true;
    }
  }
//std::cerr << vertex->reference() << " has no pending recipient " << entry(vertex)->recipients.size() << std::endl;
  return false;
}

std::shared_ptr<Event> SeededController::dispatchEvent(const SystemState* systemState) {
//std::cerr << "dispatchEvent" << std::endl;
  if ( terminationEvent ) {
    return terminationEvent;
  }

  // when dispatchEvent is called all tokens have been advanced as much as possible
  // non-decision vertices may only be pending if they are visited and must wait for 
  // the respective event, e.g. timer event, completion event

  auto getRequest = [](const Vertex* vertex, const auto& pendingDecisions) -> DecisionRequest* {
    for (const auto& [token_ptr, request_ptr] : pendingDecisions) {
      if (auto request = request_ptr.lock()) {
//std::cerr << request->token->jsonify() << "/" << vertex->reference() << std::endl;
        if (
          request->token->node == vertex->node &&
          request->token->data->at(BPMNOS::Model::ExtensionElements::Index::Instance).get().value() == vertex->instanceId
        ) {
          return request.get();
        }
      }
    }
    return nullptr;
  };

  auto waitingForSequentialPerformer = [&](const Vertex* vertex) -> bool {
    assert( vertex->parent.has_value() );
    assert( performing.contains(vertex->performer()) );
    if ( auto other = performing.at(vertex->performer()) ) {
      return other != entry(vertex);
    }
    return false;
  };

  auto hasRequest = [&](const Vertex* vertex) -> DecisionRequest* {
    if ( vertex->type == Vertex::Type::ENTRY ) {
      return getRequest(vertex,systemState->pendingEntryDecisions);
    }
    if (auto request = getRequest(vertex, systemState->pendingExitDecisions)) {
      return request;
    }
    if (auto request = getRequest(vertex, systemState->pendingMessageDeliveryDecisions)) {
      return request;
    }
    if (auto request = getRequest(vertex, systemState->pendingChoiceDecisions)) {
      return request;
    }
    return nullptr;
  };

  auto createEvent = [&](const Vertex* vertex, DecisionRequest* request) -> std::shared_ptr<Event> {
    std::shared_ptr<Event> event;
    using enum RequestType;
    if ( request->type == EntryRequest ) {
      event = createEntryEvent( systemState, request->token, vertex);
    }
    else if ( request->type == ExitRequest ) {
      event = createExitEvent( systemState, request->token, vertex);
    }
    else if ( request->type == MessageDeliveryRequest ) {
      event = createMessageDeliveryEvent( systemState, request->token, vertex);
    }
    else if ( request->type == ChoiceRequest ) {
      event = createChoiceEvent( systemState, request->token, vertex);
    }
    else {
      assert(!"Unexpected request type");
    }
    return event;
  };
//std::cerr << pendingVertices.size() <<  " pending vertices" << std::endl;
    
  auto it = pendingVertices.begin();
  while ( it != pendingVertices.end() ) {
    auto vertex = *it;
//std::cerr << "Pending: " << vertex->reference() << std::endl;
    if ( hasPendingPredecessor(vertex) ) {
//std::cerr << "Postpone (pending predecessor): " << vertex->reference() << std::endl;
      // postpone vertex because a predecessor has not yet been processed
      it++;
      continue;
    }
    else if ( flattenedGraph.dummies.contains(vertex) ) {
      // vertex is dummy 
//std::cerr << "Dummy: " << vertex->reference() << std::endl;
      it = finalizeVertexPosition(vertex);
      continue;
    }
    else if ( vertex->entry<BPMN::TypedStartEvent>() ) {
//std::cerr << "Ignore: " << vertex->reference() << std::endl;
      // ignore vertex and proceed with exit
      it++;
      continue;
    }
    else if ( vertex->exit<BPMN::TypedStartEvent>() && hasPendingPredecessor(entry(vertex)) ) {
//std::cerr << "Postpone (pending predecessor): " << vertex->reference() << std::endl;
      // postpone vertex because a predecessor has not yet been processed
      it++;
      continue;
    }
    else if ( auto request = hasRequest(vertex) ) {
//std::cerr << "Request: " << request->token->jsonify() << std::endl;
      if ( auto event = createEvent(vertex,request) ) {
//std::cerr << "Event: " << vertex->reference() << std::endl;
        return event;
      }
      else {
//std::cerr << "Postpone (infeasible): " << vertex->reference() << std::endl;
        // postpone vertex because there is no feasible way to process
        it++;
        continue;
      }
    }
    else  if ( vertex->exit<BPMN::SendTask>() ) {
      if ( hasPendingRecipient(vertex) ) {
//std::cerr << "Postpone (send task): " << vertex->reference() << std::endl;
        // postpone vertex because there is no feasible way to process
        it++;
        continue;
      }
      else {
//std::cerr << "Failed (send task): " << vertex->reference() << std::endl;
        // ensure send task follows last position
        finalizeVertexPosition( vertex );
        --lastPosition; // 
        // terminate as no solution exists
        return std::make_shared<TerminationEvent>();            
      }
    }
    else if ( vertex->exit<BPMN::MessageStartEvent>() ) {
      // all there is no request
//std::cerr << "Postpone (message start): " << vertex->reference() << std::endl;
      // postpone vertex because a predecessor has not yet been processed
      it++;
      continue;
    }
    else if ( vertex->entry<BPMN::Process>() ) {
      // wait for process to start
//std::cerr << "Wait: " << vertex->jsonify() << std::endl;
      return nullptr;
    }
    else if ( vertex->exit<BPMN::TypedStartEvent>() ) {
      // wait for trigger
//std::cerr << "Wait: " << vertex->jsonify() << std::endl;
      return nullptr;
    }
    else if ( 
      vertex->node->represents<BPMN::Activity>() &&
      vertex->node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() &&
      waitingForSequentialPerformer( vertex ) 
    ) {
//std::cerr << "Postpone (sequential activity): " << vertex->reference() << std::endl;
      // ignore vertex at sequential activity because performer is busy with other activity
      it++;
      continue;
    }
    else if ( 
      vertex->exit<BPMN::Activity>() && 
      !vertex->node->represents<BPMN::ReceiveTask>() && 
      !vertex->node->represents<BPMNOS::Model::DecisionTask>() &&
      !std::ranges::contains(pendingVertices,entry(vertex))
    ) {
//std::cerr << "Wait: " << vertex->jsonify() << std::endl;
      // wait for activity to be completed
      return nullptr;
    }
    else if ( 
      vertex->exit<BPMN::TimerCatchEvent>() && 
      !vertex->node->represents<BPMN::TimerStartEvent>() && 
      !std::ranges::contains(pendingVertices,entry(vertex))
    ) {
//std::cerr << "Wait: " << vertex->jsonify() << std::endl;
      // wait for timer to be triggered
      return nullptr;
    }
    else {
      // postpone vertex because it has no predecessors, no request, and does not incur waiting
      it++;
      continue;
    }
  }
  if ( !pendingVertices.empty() && it == pendingVertices.end() ) {
    // none of the pending decision requests is feasible, and the only
    // way for one to become feasible would be by changing the timestamp.
    // - entry request: we can assume a timer event to preceed
    // - exit request: for a task we can assume that the duration of the activity
    // to be set appropriately, for subprocesses we can assume a timer event
    // to be used
    // - choice request: we can assume that a feasible choice must always exist
    // - message delivery request: we can assume that the feasibility of a message
    // delivery is not time-dependent
//std::cerr << processedVertices.size() << "/" << pendingVertices.size()  << " - Terminate: " <<  pendingVertices.front()->reference() << std::endl;   
    return std::make_shared<TerminationEvent>();  
  }
  return nullptr;
}

CPSolution& SeededController::createSolution() {
  lastPosition = 0;
  terminationEvent.reset();
  _solution = std::make_unique<CPSolution>( model );
  initializePendingVertices();

  return *_solution.get();
}

std::list<size_t> SeededController::getSequence() const {
  std::list<size_t> sequence;

  for ( auto vertex : processedVertices ) {
    assert( vertex == flattenedGraph.vertices[vertex->index].get() );
    sequence.push_back( vertex->index + 1 );
//std::cerr << vertex->reference() << std::endl;
  }

  return sequence;
}

/*
std::optional< BPMNOS::number > SeededController::getTimestamp( const Vertex* vertex ) const {
  assert( model.status.contains(vertex) );
  assert( model.status.at(vertex).size() > BPMNOS::Model::ExtensionElements::Index::Timestamp );
//  auto timestamp = _solution->evaluate( model.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value );
  auto timestamp = _solution->getTimestamp( vertex );
  if ( timestamp ) {
    return (number)timestamp.value();
  }
  return std::nullopt;
}

// TODO: use below for a potential CPController

std::shared_ptr<Event> SeededController::createEntryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }

  return std::make_shared<EntryEvent>(token);
}

std::shared_ptr<Event> SeededController::createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }
  return std::make_shared<ExitEvent>(token);
}

std::shared_ptr<Event> SeededController::createChoiceEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }

  auto& solution = getSolution();
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  BPMNOS::Values choices;
  for ( auto& choice : extensionElements->choices ) {
    assert( choice->attribute->category == BPMNOS::Model::Attribute::Category::STATUS );
//    auto value = solution.getVariableValue( model.status.at(vertex)[choice->attribute->index].value );
    auto value = solution.getStatusValue( vertex, choice->attribute->index );
    if ( !value ) {
      // no choice value provided
      return nullptr;
    }
    choices.push_back( (number)value.value() );
  }
  return std::make_shared<ChoiceEvent>(token,std::move(choices));
}

std::shared_ptr<Event> SeededController::createMessageDeliveryEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
//std::cerr << timestamp.has_value() << std::endl;
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }
  
  for ( Vertex& sender : vertex->senders ) {
    if ( !_solution->messageFlows(&sender,vertex) ) {
      continue;
    }
    // find message with header indicating that the sender is the sending vertex
    for ( auto& [ message_ptr ] : systemState->outbox.at(sender.node->as<BPMN::FlowNode>()) ) {
      if ( auto message = message_ptr.lock();
        message &&
        message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ] == sender.instanceId
      ) {
        return std::make_shared<MessageDeliveryEvent>(token, message.get());
      }
    }
  }

  // message is not yet sent
  return nullptr;
}
*/

void SeededController::initializePendingVertices() {
//std::cerr << "initialize " << seed.size() << " pending vertices " << &model << std::endl;
  lastPosition = 0;
  terminationEvent.reset();
  pendingVertices.clear();
  processedVertices.clear();
  performing.clear();
  std::vector<double> positions(seed.size());
  size_t position = 0;
  for ( auto index : seed ) {
    
    pendingVertices.push_back( flattenedGraph.vertices[ index - 1 ].get() ); // seed indices start at 1
//std::cerr << position+1 << ". position: " << index << "/'" << flattenedGraph.vertices[ index - 1 ].reference() << "'" << std::endl;
    positions[ index - 1 ] = (double)++position;
  }
  _solution->initializePositions(positions);
}
