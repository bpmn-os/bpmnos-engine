#include "CPSolution.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"
#include <iostream>

using namespace BPMNOS::Execution;

CPSolution::CPSolution(const CPModel& cp)
 : cp(cp)
 , flattenedGraph( cp.flattenedGraph )
 , _solution( cp.getModel() )
 , lastPosition(0)
{
  // set collection evaluator
  _solution.setCollectionEvaluator( 
    [](double value) ->  std::expected< std::reference_wrapper<const std::vector<double> >, std::string >  {
      if ( value < 0 || value >= (double)collectionRegistry.size() ) {
        return std::unexpected("Unable to determine collection for index " + BPMNOS::to_string(value) );
      }
      return collectionRegistry[(size_t)value];
    }
  );

  // add evaluators for lookup tables
  for ( auto& lookupTable : cp.getScenario().model->lookupTables ) {
    _solution.addEvaluator( 
      lookupTable->name,
      [&lookupTable](const std::vector<double>& operands) -> double {
        return lookupTable->at(operands);
      }
    );
  }
}

void CPSolution::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Execution::Observable::Type::Event,
    Execution::Observable::Type::Token
  );
}

void CPSolution::unsubscribe(Engine* engine) {
  engine->removeSubscriber(this, 
    Execution::Observable::Type::Event,
    Execution::Observable::Type::Token
  );
}

void CPSolution::notice(const Observable* observable) {
  if ( observable->getObservableType() == Execution::Observable::Type::Token ) {
    synchronize( static_cast<const Token*>(observable) );
  }
  else {
    assert( observable->getObservableType() == Execution::Observable::Type::Event );
    synchronize( static_cast<const Event*>(observable) );
  }
}

void CPSolution::synchronize(const Token* token) {
  if ( token->state == Token::State::ENTERED ) {
    // entry at node
    if ( auto vertex = flattenedGraph.getVertex(token);
      vertex &&
      !vertex->node->represents<BPMN::TypedStartEvent>() &&
      !( vertex->node->represents<BPMN::CatchEvent>() && vertex->inflows.front().second.node->represents<BPMN::EventBasedGateway>() )
    ) {
      _solution.setVariableValue( cp.visit.at(vertex), true );
    }
    // TODO: set position
  }
  else if ( token->node && token->state == Token::State::COMPLETED && token->node->represents<BPMN::TypedStartEvent>() ) {
    auto vertex = flattenedGraph.getVertex(token);
    assert( vertex );
    // event-subprocess is triggered
    _solution.setVariableValue( cp.visit.at(vertex), true );
  }
  else if ( token->node && token->state == Token::State::WITHDRAWN && token->node->represents<BPMN::TypedStartEvent>() ) {
    if ( auto vertex = flattenedGraph.getVertex(token) ) {
      // event-subprocess is not triggered
      _solution.setVariableValue( cp.visit.at(vertex), false );
    }
  }
  else if ( 
    ( !token->node && token->state == Token::State::DONE ) || // Process
    ( token->node && token->state == Token::State::EXITING && !token->node->represents<BPMN::TypedStartEvent>() ) || // Activity
    ( token->node && token->state == Token::State::COMPLETED && token->node->represents<BPMN::CatchEvent>() && !token->node->represents<BPMN::ReceiveTask>() )
  ) {
    // exit of node
    auto vertex = flattenedGraph.getVertex( token );
    assert( vertex );
    
    if ( vertex->node->represents<BPMN::CatchEvent>() && vertex->inflows.front().second.node->represents<BPMN::EventBasedGateway>() ) {
      _solution.setVariableValue( cp.visit.at(vertex), true );
    }

    // TODO: set position

    auto entryVertex = entry(vertex);
    if ( 
      entryVertex->inflows.size() == 1 && 
      entryVertex->inflows.front().second.node->represents<BPMN::EventBasedGateway>()
    ) {
      assert( vertex->exit<BPMN::CatchEvent>() );
      auto gateway = &entryVertex->inflows.front().second;
      setTriggeredEvent( gateway, entryVertex );
    }
  }
}

void CPSolution::synchronize(const Event* event) {
  if ( dynamic_cast<const EntryEvent*>(event) ) {
//std::cerr << "Entry: " << event->jsonify() << std::endl;
    auto vertex = flattenedGraph.getVertex( event->token );
    if ( !flattenedGraph.dummies.contains(vertex) ) {
      auto timestamp = event->token->owner->systemState->getTime();
      setTimestamp(vertex, timestamp );
    }
  }
  else if ( dynamic_cast<const ExitEvent*>(event) ) {
//std::cerr << "Exit: " << event->jsonify() << std::endl;
    auto vertex = flattenedGraph.getVertex( event->token );
    if ( !flattenedGraph.dummies.contains(vertex) ) {
      auto timestamp = event->token->owner->systemState->getTime();
      setTimestamp(vertex, timestamp );
    }
  }
  else if ( auto messageDeliveryEvent = dynamic_cast<const MessageDeliveryEvent*>(event) ) {
//std::cerr << "Message delivery: " << event->jsonify() << std::endl;
    auto vertex = flattenedGraph.getVertex( event->token );
    // set timestamp of message delivery
    auto timestamp = event->token->owner->systemState->getTime();
    if ( vertex->node->represents<BPMN::ReceiveTask>() ) {
      setLocalStatusValue(vertex,BPMNOS::Model::ExtensionElements::Index::Timestamp,timestamp);
    }
    else {
      setTimestamp(vertex,timestamp);
    }
    auto message = messageDeliveryEvent->message.lock();
    assert( message );
    auto senderId = message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ].value();
    assert( flattenedGraph.vertexMap.contains({senderId,{},message->origin}) );
    // TODO: sender must not be within loop
    auto& [senderEntry,senderExit] = flattenedGraph.vertexMap.at({senderId,{},message->origin}); 
    setMessageDeliveryVariableValues(&senderEntry,vertex,timestamp);
  }
  else if ( auto choiceEvent = dynamic_cast<const ChoiceEvent*>(event) ) {
//std::cerr << "Choice: " << event->jsonify() << std::endl;
    auto vertex = flattenedGraph.getVertex( event->token );
    // set timestamp of choice
    auto timestamp = event->token->owner->systemState->getTime();
    setLocalStatusValue(vertex,BPMNOS::Model::ExtensionElements::Index::Timestamp,timestamp);
    // apply choices 
    auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert( extensionElements );
    assert( extensionElements->choices.size() == choiceEvent->choices.size() );
    for (size_t i = 0; i < extensionElements->choices.size(); i++) {
      assert( choiceEvent->choices[i].has_value() );
      setLocalStatusValue(vertex,extensionElements->choices[i]->attribute->index,choiceEvent->choices[i].value());
    }
  }
}



void CPSolution::setMessageDeliveryVariableValues( const Vertex* sender, const Vertex* recipient, BPMNOS::number timestamp ) {
//std::cerr << "Sen:" << sender->jsonify() << std::endl;  
//std::cerr << "Rec:" << recipient->jsonify() << std::endl;  
  for ( const Vertex& candidate : sender->recipients ) {
    assert( cp.messageFlow.contains({sender,&candidate}) );
//std::cerr << "message_{" << sender->reference() << " -> " << candidate.reference() << "} := " << (&candidate == recipient) << std::endl; 
    _solution.setVariableValue( cp.messageFlow.at({sender,&candidate}), (double)(&candidate == recipient) );
  }
  for ( const Vertex& candidate : recipient->senders ) {
    assert( cp.messageFlow.contains({&candidate,recipient}) );
    _solution.setVariableValue( cp.messageFlow.at({&candidate,recipient}), (double)(&candidate == sender) );
//std::cerr << "message_{" << candidate.reference() << " -> " << recipient->reference() << "} := " << (&candidate == sender) << std::endl; 
  }
  // set message delivery time
  if ( recipient->node->represents<BPMN::ReceiveTask>() || recipient->node->represents<BPMN::MessageStartEvent>() ) {
    setLocalStatusValue( recipient, BPMNOS::Model::ExtensionElements::Index::Timestamp, (double)timestamp );
  }
  else {
    _solution.setVariableValue( cp.status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, (double)timestamp );
  }

  // set recipient content variables
  auto& recipientContentMap = cp.messageContent.at(recipient);
  auto& senderContentMap = cp.messageContent.at(sender);
  for ( auto& [key, recipientContent ] : recipientContentMap ) {
    assert( senderContentMap.contains(key) );
    auto& senderContent = senderContentMap.at(key);
    CPModel::AttributeEvaluation evaluation(
      _solution.evaluate( senderContent.defined ),
      _solution.evaluate( senderContent.value )
    );
    if ( evaluation ) {
      _solution.setVariableValue( recipientContent.defined, evaluation.defined() );
      _solution.setVariableValue( recipientContent.value, evaluation.value() );
    }
  }
}

std::optional< BPMN::Activity::LoopCharacteristics> CPSolution::getLoopCharacteristics(const Vertex* vertex) const {
  auto activity = vertex->node->represents<BPMN::Activity>();
  if ( !activity ) {
    return std::nullopt;
  }
  return activity->loopCharacteristics;
}

void CPSolution::unvisitEntry(const Vertex* vertex) {
//std::cerr << "unvisited " << vertex->reference() << std::endl;
//  _solution.setVariableValue( cp.position.at(vertex), (double)position );
  assert( cp.visit.contains(vertex) );
  _solution.setVariableValue( cp.visit.at(vertex), false);
  assert( cp.status.contains(vertex) );
  _solution.setVariableValue( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, 0.0 );

  if ( vertex->node->represents<BPMN::MessageThrowEvent>() ) {
    for ( const Vertex& candidate : vertex->recipients ) {
      assert( candidate.exit<BPMN::MessageCatchEvent>() );
      assert( cp.messageFlow.contains({vertex,&candidate}) );
      _solution.setVariableValue( cp.messageFlow.at({vertex,&candidate}), (double)false );
    }
  }

  if ( 
    vertex->node->represents<BPMN::Task>() &&
    !(vertex->node->represents<BPMN::TypedStartEvent>() || vertex->node->represents<BPMN::ReceiveTask>() || vertex->node->represents<BPMNOS::Model::DecisionTask>())
  ) {
    // if visited, operators may modify data and globals upon entry
    // if unvisited, data and globals are not changed
    for ( size_t i = 0; i < vertex->dataOwners.size(); i++ ) {
      Vertex& dataOwner = vertex->dataOwners[i];
      if ( flattenedGraph.modifiesData( *vertex, dataOwner ) ) {
        auto index = (size_t)_solution.evaluate( cp.dataIndex.at(vertex)[i] ).value();

        auto extensionElements = dataOwner.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        assert( extensionElements );
        for ( auto& attribute : extensionElements->data ) {
          auto defined = _solution.evaluate( cp.data.at({&dataOwner,attribute.get()}).defined[index] ).value();
          auto value = _solution.evaluate( cp.data.at({&dataOwner,attribute.get()}).value[index] ).value();
          _solution.setVariableValue( cp.data.at({&dataOwner,attribute.get()}).defined[index+1], defined );
          _solution.setVariableValue( cp.data.at({&dataOwner,attribute.get()}).value[index+1], value );
        }
      }
    }
    if ( flattenedGraph.modifiesGlobals( *vertex ) ) {
      auto index = (size_t)_solution.evaluate( cp.globalIndex.at(vertex) ).value();
      for ( size_t attributeIndex = 0; attributeIndex < cp.globals.size(); attributeIndex++ ) {
        auto defined = _solution.evaluate( cp.globals.at(attributeIndex).defined[index] ).value();
        auto value = _solution.evaluate( cp.globals.at(attributeIndex).value[index] ).value();
        _solution.setVariableValue( cp.globals.at(attributeIndex).defined[index+1], defined );
        _solution.setVariableValue( cp.globals.at(attributeIndex).value[index+1], value );
      }
    }
  }
}

void CPSolution::unvisitExit(const Vertex* vertex) {
//std::cerr << "unvisited " << vertex->reference() << std::endl;
  assert( cp.visit.contains(vertex) );
//  _solution.setVariableValue( cp.position.at(vertex), (double)position );
  _solution.setVariableValue( cp.visit.at(vertex), false);
  assert( cp.status.contains(vertex) );
  _solution.setVariableValue( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, 0.0 );

  if ( vertex->node->represents<BPMN::MessageCatchEvent>() ) {
    for ( const Vertex& candidate : vertex->senders ) {
      assert( candidate.entry<BPMN::MessageThrowEvent>() );
      assert( cp.messageFlow.contains({&candidate,vertex}) );
      _solution.setVariableValue( cp.messageFlow.at({&candidate,vertex}), (double)false );
    }

    if ( cp.locals.contains(vertex) ) {
      // only receive tasks and message start events have locals
      setLocalStatusValue( vertex, BPMNOS::Model::ExtensionElements::Index::Timestamp, 0.0 );
    }

    assert( cp.messageContent.contains(vertex) );
    for ( auto& [_, contentVariables] : cp.messageContent.at(vertex) ) {
      _solution.setVariableValue( contentVariables.defined, (double)false );
      _solution.setVariableValue( contentVariables.value, 0.0 );
    }
  }

  if ( vertex->node->represents<BPMNOS::Model::DecisionTask>() ) {
    setLocalStatusValue( vertex, BPMNOS::Model::ExtensionElements::Index::Timestamp, 0.0 );
    // set choice values to zero
    auto  extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
      if ( cp.findChoice(extensionElements->choices, attribute) ) {
        // attribute set by choice
        setLocalStatusValue( vertex, attribute->index, 0.0 );
      }
    }
  }

  if ( vertex->node->represents<BPMN::TypedStartEvent>() || vertex->node->represents<BPMN::ReceiveTask>() || vertex->node->represents<BPMNOS::Model::DecisionTask>() ) {
    // if visited, operators may modify data and globals upon exit
    // if unvisited, data and globals are not changed
    for ( size_t i = 0; i < vertex->dataOwners.size(); i++ ) {
      Vertex& dataOwner = vertex->dataOwners[i];
      if ( flattenedGraph.modifiesData( *vertex, dataOwner ) ) {
        auto index = (size_t)_solution.evaluate( cp.dataIndex.at(exit(vertex))[i] ).value();

        auto extensionElements = dataOwner.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        assert( extensionElements );
        for ( auto& attribute : extensionElements->data ) {
          auto defined = _solution.evaluate( cp.data.at({&dataOwner,attribute.get()}).defined[index-1] ).value();
          auto value = _solution.evaluate( cp.data.at({&dataOwner,attribute.get()}).value[index-1] ).value();
          _solution.setVariableValue( cp.data.at({&dataOwner,attribute.get()}).defined[index], defined );
          _solution.setVariableValue( cp.data.at({&dataOwner,attribute.get()}).value[index], value );
        }
      }
    }
    if ( flattenedGraph.modifiesGlobals( *vertex ) ) {
      auto index = (size_t)_solution.evaluate( cp.globalIndex.at(exit(vertex)) ).value();
      for ( size_t attributeIndex = 0; attributeIndex < cp.globals.size(); attributeIndex++ ) {
        auto defined = _solution.evaluate( cp.globals.at(attributeIndex).defined[index-1] ).value();
        auto value = _solution.evaluate( cp.globals.at(attributeIndex).value[index-1] ).value();
        _solution.setVariableValue( cp.globals.at(attributeIndex).defined[index], defined );
        _solution.setVariableValue( cp.globals.at(attributeIndex).value[index], value );
      }
    }
  }
}

/*
void CPSolution::visit(const Vertex* vertex) {
//std::cerr << vertex->reference() << "/" << cp.visit.begin()->second.stringify() << std::endl;
//std::cerr << vertex << "/" << cp.visit.begin()->first  << std::endl;
  // check visit
  assert( cp.visit.contains(vertex) );
  auto visitEvaluation = _solution.evaluate( cp.visit.at(vertex) );
  if ( !visitEvaluation ) {
    // set solution value
    _solution.setVariableValue( cp.visit.at(vertex), true );
  }
  else if ( visitEvaluation && visitEvaluation.value() != true ) {
    throw std::logic_error("CPSolution: vertex '" + vertex->reference() +"' contradictingly visited in solution");
  }
}

void CPSolution::visitEntry(const Vertex* vertex, double timestamp) {
//  _solution.setVariableValue( cp.position.at(vertex), (double)position );
  assert( cp.visit.contains(vertex) );
  _solution.setVariableValue(cp.visit.at(vertex), true);
  assert( cp.status.contains(vertex) );
  _solution.setVariableValue( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, timestamp );
}

void CPSolution::visitExit(const Vertex* vertex, double timestamp) {
//  _solution.setVariableValue( cp.position.at(vertex), (double)position );
//  _solution.setVariableValue(cp.visit.at(vertex), true);
  assert( cp.status.contains(vertex) );
  _solution.setVariableValue( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, timestamp );
}
*/

void CPSolution::synchronizeStatus(const BPMNOS::Values& status, const CPSolution::Vertex* vertex) {
  assert( cp.status.contains(vertex) );
  auto& statusVariables = cp.status.at(vertex);
  assert( status.size() == statusVariables.size() );
  for (size_t i = 0; i < statusVariables.size(); i++) {
    CPModel::AttributeEvaluation evaluation(
      _solution.evaluate( statusVariables[i].defined ),
      _solution.evaluate( statusVariables[i].value )
    );
    if ( status[i].has_value() ) {
      if ( !evaluation ) {
        // set solution value
        _solution.setVariableValue( statusVariables[i].defined, true );
        _solution.setVariableValue( statusVariables[i].value, (double)status[i].value() );
      }
      else if ( 
        !evaluation.defined()  ||
        evaluation.value() != status[i].value()
      ) {

std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << std::endl;
//std::cerr << statusVariables[i].defined.stringify() << std::endl;
//std::cerr << statusVariables[i].value.stringify() << std::endl;
//std::cerr << "Model: " << cp.stringify() << std::endl;
//std::cerr << "Solution: " <<  _solution.stringify() << std::endl;
        throw std::logic_error("CPSolution: '" + _solution.stringify(statusVariables[i].defined) + "' or '" + _solution.stringify(statusVariables[i].value) + "' inconsistent with given status" );
      }
    }
    else {
      if ( !evaluation ) {
        // set solution value
        _solution.setVariableValue( statusVariables[i].defined, false );
        _solution.setVariableValue( statusVariables[i].value, 0.0 );
      }
      else if ( 
        evaluation.defined()  ||
        evaluation.value() != 0.0
      ) {
std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << std::endl;
        throw std::logic_error("CPSolution: '" + _solution.stringify(statusVariables[i].defined) + "' or '" + _solution.stringify(statusVariables[i].value) + "' inconsistent with given status" );
      }
    }
  }
}

void CPSolution::synchronizeData(const BPMNOS::SharedValues& data, const CPSolution::Vertex* vertex) {
  assert( cp.dataIndex.contains(vertex) );
  auto& dataIndices = cp.dataIndex.at(vertex);
  assert( dataIndices.size() == vertex->dataOwners.size() );
  // iterate over the data indices for each data owner
  for ( size_t i = 0; i < dataIndices.size(); i++ ) {
    auto indexEvaluation = _solution.evaluate( dataIndices[i] );
    if ( !indexEvaluation ) {
std::cerr << dataIndices[i].stringify() << std::endl;
      throw std::logic_error("CPSolution: Unable to determine data index for '" + vertex->reference() + "\n'" + indexEvaluation.error());
    }
    auto index = (size_t)indexEvaluation.value();
    auto &ownerVertex = vertex->dataOwners[i].get();
    assert( ownerVertex.entry<BPMN::Scope>() );
//std::cerr << "set data[" << ownerVertex.shortReference() << ", "<< index << "] for " << vertex->reference() << std::endl;
    auto scope = ownerVertex.node;
    auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    for ( auto& attribute : extensionElements->data ) {
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Instance ) {
        continue;
      }
      auto& indexedAttributeVariables = cp.data.at({&ownerVertex,attribute.get()});
      // override solution value (positions and indices may have changed)
      if ( data[attribute->index].get().has_value() ) {
        _solution.setVariableValue( indexedAttributeVariables.defined[index], true );
        _solution.setVariableValue( indexedAttributeVariables.value[index], (double)data[attribute->index].get().value() );
      }
      else {
        _solution.setVariableValue( indexedAttributeVariables.defined[index], false );
        _solution.setVariableValue( indexedAttributeVariables.value[index], 0.0 );
      }
    }
  }
}

void CPSolution::synchronizeGlobals(const BPMNOS::Values& globals, const CPSolution::Vertex* vertex) {
  assert( cp.globalIndex.contains(vertex) );
  auto indexEvaluation = _solution.evaluate( cp.globalIndex.at(vertex) );
  if ( !indexEvaluation ) {
    throw std::logic_error("CPSolution: Unable to determine global index for '" + vertex->reference() + "\n'" + indexEvaluation.error());
  }
  auto index = (size_t)indexEvaluation.value();
  for ( size_t attributeIndex = 0; attributeIndex < globals.size(); attributeIndex++ ) {
    auto& indexedAttributeVariables = cp.globals[attributeIndex];
    // override solution value (positions and indices may have changed)
    if ( globals[attributeIndex].has_value() ) {
      _solution.setVariableValue( indexedAttributeVariables.defined[index], true );
      _solution.setVariableValue( indexedAttributeVariables.value[index], (double)globals[attributeIndex].value() );
    }
    else {
      _solution.setVariableValue( indexedAttributeVariables.defined[index], false );
      _solution.setVariableValue( indexedAttributeVariables.value[index], 0.0 );
    }
  }
}

/*
const CP::Solution& CPSolution::getSolution() const {
  return _solution;
}
*/

std::vector<size_t> CPSolution::getSequence() const {
  std::vector<size_t> sequence(flattenedGraph.vertices.size());
  for ( size_t i = 0; i < flattenedGraph.vertices.size(); i++ ) {
    sequence[ (size_t)_solution.getVariableValue( cp.position.at( &flattenedGraph.vertices[i] ) ).value() -1 ] = i + 1;
  }
  
  return sequence;
}

size_t CPSolution::getPosition(const Vertex* vertex) const {
  assert( cp.position.contains( vertex ) );
  return (size_t)_solution.getVariableValue( cp.position.at( vertex ) ).value_or(0);
}

void CPSolution::initializePositions(const std::vector<double>& positions) {
  assert( positions.size() == flattenedGraph.vertices.size() );
  assert( cp.getModel().getSequences().size() == 1 );
  auto& sequenceVariable = cp.getModel().getSequences().front();
  _solution.setSequenceValues( sequenceVariable, positions );
}

void CPSolution::setPosition(const Vertex* vertex, size_t position) {
  assert( cp.position.contains( vertex ) );
  assert( _solution.getVariableValue( cp.position.at(vertex) ).has_value() );
  auto priorPosition = (size_t)_solution.getVariableValue( cp.position.at(vertex) ).value();
//std::cerr << "change position(" << vertex->reference() << ") from " <<  priorPosition << " to " << position << std::endl;
  assert( position <= priorPosition );
  if ( position < priorPosition ) {
    for ( auto& other : flattenedGraph.vertices ) {
      if ( &other == vertex ) {
        continue;
      }
      assert( _solution.getVariableValue( cp.position.at(&other) ).has_value() );
      auto otherPosition = (size_t)_solution.getVariableValue( cp.position.at(&other) ).value();
      if ( otherPosition >= position && otherPosition < priorPosition ) {
        // increment position of other vertex 
        _solution.setVariableValue( cp.position.at(&other), (double)++otherPosition );
      }
    }
  }
  // change vertex position
  _solution.setVariableValue( cp.position.at(vertex), (double)position );
}

void CPSolution::finalizePosition(const Vertex* vertex) {
  setPosition(vertex, ++lastPosition);
  for ( auto& [_,other] : vertex->outflows ) {
    auto isVisited = _solution.evaluate( cp.visit.at(&other) );
    if ( isVisited.has_value() && !isVisited.value() ) {
      // successor is not visited
    }
  }
}

bool CPSolution::isVisited(const Vertex* vertex) const {
  assert( cp.visit.contains( vertex ) );
  return _solution.evaluate( cp.visit.at( vertex ) ).value_or(false);
}

bool CPSolution::isUnvisited(const Vertex* vertex) const {
  assert( cp.visit.contains( vertex ) );
  return !_solution.evaluate( cp.visit.at( vertex ) ).value_or(true);
}

bool CPSolution::messageFlows( const Vertex* sender, const Vertex* recipient ) {
  assert( cp.messageFlow.contains( {sender,recipient} ) );
  return _solution.evaluate( cp.messageFlow.at( {sender,recipient} ) ).value_or(false);
}


std::optional< BPMNOS::number > CPSolution::getTimestamp( const Vertex* vertex ) const {
  assert( cp.status.contains(vertex) );
  assert( cp.status.at(vertex).size() > BPMNOS::Model::ExtensionElements::Index::Timestamp );
  auto timestamp = _solution.evaluate( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value );
  if ( timestamp ) {
    return (number)timestamp.value();
  }
  return std::nullopt;
}

void CPSolution::setTimestamp( const Vertex* vertex, BPMNOS::number timestamp ) {
  _solution.setVariableValue( cp.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, (double)timestamp );
}

std::optional< BPMNOS::number > CPSolution::getStatusValue( const Vertex* vertex, size_t attributeIndex ) const {
  if ( !_solution.evaluate( cp.status.at( vertex )[attributeIndex].defined ).has_value() ||  !_solution.evaluate( cp.status.at( vertex )[attributeIndex].defined ).value() ) {
    return std::nullopt;  
  }
  return _solution.evaluate( cp.status.at(vertex)[attributeIndex].value ).value(); 
}

void CPSolution::setTriggeredEvent( const Vertex* gateway, const Vertex* event ) {
//std::cerr << gateway->reference() << "/" << event->reference() << std::endl;
  assert( event->entry<BPMN::CatchEvent>() );
  assert( gateway->exit<BPMN::EventBasedGateway>() );
  for ( auto& [ _, target ] : gateway->outflows ) {
    _solution.setVariableValue( cp.tokenFlow.at({gateway,&target}), ( &target == event ) );
//std::cerr << "Token flow " << gateway.reference() << " to " << target.reference() << " = " << ( &target == entryVertex ) << std::endl;
  } 
}


void CPSolution::setLocalStatusValue( const Vertex* vertex, size_t attributeIndex, BPMNOS::number value ) {
  auto& initialStatus = std::get<0>(cp.locals.at(vertex)[0]);
  _solution.setVariableValue( initialStatus[attributeIndex].value, (double)value );
}

