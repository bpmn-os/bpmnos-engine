#include "CPController.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "execution/engine/src/Engine.h"
#include <limex_handle.h>
#include <iostream>

using namespace BPMNOS::Execution;

CPController::CPController(const BPMNOS::Model::Scenario* scenario, Config config)
 : scenario(scenario)
 , config(std::move(config))
 , flattenedGraph(FlattenedGraph(scenario))
 , model(CP::Model::ObjectiveSense::MAXIMIZE)
 , _solution(nullptr)
{
std::cerr << "Flattened graph: " << flattenedGraph.jsonify().dump() << std::endl;
  // TODO: add callables for lookup tables

  // add callables for lookup tables
  for ( auto& lookupTable : scenario->model->lookupTables ) {
    limexHandle.add(
      lookupTable->name, 
      [&lookupTable](const std::vector<CP::Expression>& args) -> CP::Expression
      {
        std::vector<CP::Operand> operands = { CP::Expression::getCustomIndex(lookupTable->name) };
        operands.insert(operands.end(), args.begin(), args.end());
        return CP::Expression(CP::Expression::Operator::custom, std::move(operands));
      }
    );
  }

std::cerr << "Limex handles: ";
for ( auto name : limexHandle.getNames() ) {
  std::cerr << name << ", ";
}
std::cerr << std::endl;

  createCP();
}

void CPController::connect(Mediator* mediator) {
  readyHandler.connect(mediator);
  completionHandler.connect(mediator);
  Controller::connect(mediator);
}

void CPController::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Execution::Observable::Type::Token
  );
}

void CPController::setMessageFlowVariableValue( const Vertex* sender, const Vertex* recipient ) {
  for ( const Vertex& candidate : sender->recipients ) {
    _solution->setVariableValue( messageFlow.at(std::make_pair(sender,&candidate)), (double)(&candidate == recipient) );
  }
  for ( const Vertex& candidate : recipient->senders ) {
    _solution->setVariableValue( messageFlow.at({&candidate,recipient}), (double)(&candidate == sender) );
  }
}

const FlattenedGraph::Vertex* CPController::getVertex( const Token* token ) const {
  auto node = token->node ? token->node->as<BPMN::Node>() : token->owner->process->as<BPMN::Node>();
  assert( flattenedGraph.loopIndexAttributes.contains(node) );
  std::vector< size_t > loopIndices;
  for ( auto attribute : flattenedGraph.loopIndexAttributes.at(node)  ) {
    assert( token->status.at(attribute->index).has_value() );
    loopIndices.push_back( (size_t)token->status.at(attribute->index).value() );
  }
  auto instanceId = token->data->at(BPMNOS::Model::ExtensionElements::Index::Instance).get().value();
  assert( flattenedGraph.vertexMap.contains({instanceId,loopIndices,node}) );
  auto& [entry,exit] = flattenedGraph.vertexMap.at({instanceId,loopIndices,node});

  return (token->state == Token::State::ENTERED) ? &entry : &exit;
}

std::optional< BPMN::Activity::LoopCharacteristics> CPController::getLoopCharacteristics(const Vertex* vertex) const {
  auto activity = vertex->node->represents<BPMN::Activity>();
  if ( !activity ) {
    return std::nullopt;
  }
  return activity->loopCharacteristics;
}

void CPController::validate(const Token* token) {
  if ( terminationEvent ) {
    return;
  }
std::cerr << "Validate: " << token->jsonify() << std::endl;    

  if ( token->state == Token::State::FAILED ) {
    terminationEvent = std::make_shared<TerminationEvent>();
  }
  
  if ( !token->node && token->state != Token::State::ENTERED && token->state != Token::State::DONE ) {
    // token at process, but with irrelevant state
    return;
  }
  if ( token->node && token->state != Token::State::ENTERED && token->state != Token::State::EXITING ) {
    // token at flow node, but with irrelevant state
    return;
  }

  auto vertex = getVertex(token);

std::cerr << "Validate vertex " << vertex->reference() << std::endl;    
//std::cerr << "Validate visit" << std::endl;    
  // check visit
  auto visitEvaluation = _solution->evaluate( visit.at(vertex) );
  if ( !visitEvaluation ) {
    // set solution value
    _solution->setVariableValue( visit.at(vertex), true );
  }
  else if ( visitEvaluation && visitEvaluation.value() != true ) {
    throw std::logic_error("CPController: vertex '" + vertex->reference() +"' not visited in solution");
  }
    
//std::cerr << "Validate status" << std::endl;    
  // check status
  auto& statusVariables = status.at(vertex);
  assert( token->status.size() == statusVariables.size() );
  for (size_t i = 0; i < statusVariables.size(); i++) {
    AttributeEvaluation evaluation(
      _solution->evaluate( statusVariables[i].defined ),
      _solution->evaluate( statusVariables[i].value )
    );
    if ( token->status[i].has_value() ) {
      if ( !evaluation ) {
        // set solution value
        _solution->setVariableValue( statusVariables[i].defined, true );
        _solution->setVariableValue( statusVariables[i].value, (double)token->status[i].value() );
      }
      else if ( 
        !evaluation.defined()  ||
        evaluation.value() != token->status[i].value()
      ) {
std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << std::endl;
        throw std::logic_error("CPController: '" + _solution->stringify(statusVariables[i].defined) + "' or '" + _solution->stringify(statusVariables[i].value) + "' inconsistent with " + token->jsonify().dump() );
      }
    }
    else {
      if ( !evaluation ) {
        // set solution value
        _solution->setVariableValue( statusVariables[i].defined, false );
        _solution->setVariableValue( statusVariables[i].value, 0.0 );
      }
      else if ( 
        evaluation.defined()  ||
        evaluation.value() != 0.0
      ) {
std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << std::endl;
        throw std::logic_error("CPController: '" + _solution->stringify(statusVariables[i].defined) + "' or '" + _solution->stringify(statusVariables[i].value) + "' inconsistent with " + token->jsonify().dump());
      }
    }
  }

//std::cerr << "Validate data" << std::endl;    
  // check data
  auto& dataIndices = dataIndex.at(vertex);
  assert( dataIndices.size() == vertex->dataOwners.size() );
  for ( size_t i = 0; i < dataIndices.size(); i++ ) {
    auto indexEvaluation = _solution->evaluate( dataIndices[i] );
    if ( !indexEvaluation ) {
      throw std::logic_error("CPController: Unable to determine data index for '" + vertex->reference() + "\n'" + indexEvaluation.error());
    }
    auto index = (size_t)indexEvaluation.value();
    auto &ownerVertex = vertex->dataOwners[i].get();
    assert( ownerVertex.entry<BPMN::Scope>() );
    auto scope = ownerVertex.node;
    auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    for ( auto& attribute : extensionElements->data ) {
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Instance ) {
        continue;
      }
      IndexedAttributeVariables& indexedAttributeVariables = data.at({&ownerVertex,attribute.get()});
      AttributeEvaluation evaluation(
        _solution->evaluate( indexedAttributeVariables.defined[index] ),
        _solution->evaluate( indexedAttributeVariables.value[index] )
      );
      if ( token->data->at(attribute->index).get().has_value() ) {
        if ( !evaluation ) {
          // set solution value
          _solution->setVariableValue( indexedAttributeVariables.defined[index], true );
          _solution->setVariableValue( indexedAttributeVariables.value[index], (double)token->data->at(attribute->index).get().value() );
        }
        else if ( 
          !evaluation.defined()  ||
          evaluation.value() != token->data->at(attribute->index).get().value()
        ) {
std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << " != " << token->data->at(attribute->index).get().value() << std::endl;
          throw std::logic_error("CPController: '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' or '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' inconsistent with " + token->jsonify().dump());
        }
      }
      else {
        if ( !evaluation ) {
          // set solution value
          _solution->setVariableValue( indexedAttributeVariables.defined[index], false );
          _solution->setVariableValue( indexedAttributeVariables.value[index], 0.0 );
        }
        else if ( 
          evaluation.defined()  ||
          evaluation.value() != 0.0
        ) {
std::cerr << "defined: " << (evaluation.defined() ? "true" : "false") << ", value: " << evaluation.value() << " != " << token->data->at(attribute->index).get().value() << std::endl;
          throw std::logic_error("CPController: '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' or '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' inconsistent with " + token->jsonify().dump());
        }
      }
    }
  }
    
//std::cerr << "Validate globals" << std::endl;    
  // check globals
  auto indexEvaluation = _solution->evaluate( globalIndex.at(vertex) );
  if ( !indexEvaluation ) {
    throw std::logic_error("CPController: Unable to determine data index for '" + vertex->reference() + "\n'" + indexEvaluation.error());
  }
  auto index = (size_t)indexEvaluation.value();
  for ( size_t attributeIndex = 0; attributeIndex < token->globals.size(); attributeIndex++ ) {
    IndexedAttributeVariables& indexedAttributeVariables = globals[attributeIndex];
    AttributeEvaluation evaluation(
      _solution->evaluate( indexedAttributeVariables.defined[index] ),
      _solution->evaluate( indexedAttributeVariables.value[index] )
    );

    if ( token->globals[attributeIndex].has_value() ) {
      if ( !evaluation ) {
        // set solution value
        _solution->setVariableValue( indexedAttributeVariables.defined[index], true );
        _solution->setVariableValue( indexedAttributeVariables.value[index], (double)token->globals[attributeIndex].value() );
      }
      else if ( 
        !evaluation.defined()  ||
        evaluation.value() != token->globals[attributeIndex].value()
      ) {
        throw std::logic_error("CPController: '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' or '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' inconsistent with " + token->jsonify().dump());
      }
    }
    else {
      if ( !evaluation ) {
        // set solution value
        _solution->setVariableValue( indexedAttributeVariables.defined[index], false );
        _solution->setVariableValue( indexedAttributeVariables.value[index], 0.0 );
      }
      else if ( 
        evaluation.defined()  ||
        evaluation.value() != 0.0
      ) {
        throw std::logic_error("CPController: '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' or '" + _solution->stringify(indexedAttributeVariables.defined[index]) + "' inconsistent with " + token->jsonify().dump());
      }
    }
  }
}

void CPController::notice(const Observable* observable) {
  Controller::notice(observable);
  
  if( observable->getObservableType() ==  Execution::Observable::Type::Token ) {
    validate( static_cast<const Token*>(observable) );
  }
}

std::shared_ptr<Event> CPController::dispatchEvent(const SystemState* systemState) {
  if ( terminationEvent ) {
    return terminationEvent;
  }
  
  
  while ( !decisionQueue.empty() ) {
    auto& [ type, vertex ] = decisionQueue.front();
    try {
      if ( flattenedGraph.dummies.contains(vertex) || !_solution->evaluate( visit.at( vertex ) ) ) {
        // no decision to be made for vertex
std::cerr << "Skip: " << vertex->reference() << std::endl;
        decisionQueue.pop();
        continue;
      }
    }
    catch(...) {
      throw std::runtime_error("CPController: failed determining whether '" + vertex->reference() + "' is visited or not");
    }
    // exit while if decision is not skipped
    break;
  }

  if ( decisionQueue.empty() ) {
    return nullptr;
  }
  auto& [ type, vertex ] = decisionQueue.front();
std::cerr << "CPController::dispatchEvent: " << vertex->reference() << std::endl;

  auto getToken = [&vertex](const auto& pendingDecisions) -> Token* {
    for (const auto& [token_ptr, request_ptr] : pendingDecisions) {
      if (auto token = token_ptr.lock()) {
        if (
          token->node == vertex->node &&
          token->data->at(BPMNOS::Model::ExtensionElements::Index::Instance).get().value() == vertex->instanceId
        ) {
          return token.get();
        }
      }
    }
    return nullptr;
  };


  std::shared_ptr<Event> event = nullptr;
  using enum RequestType;
std::cerr << "type: " << (int)type << std::endl;
  switch ( type ) {
    case EntryRequest:
    {
std::cerr << "pendingEntryDecisions: " << !systemState->pendingEntryDecisions.empty() << std::endl;
      if ( auto token = getToken(systemState->pendingEntryDecisions) ) {
        event = createEntryEvent( systemState, token, vertex);
      }
      // no event is create if decision is not yet requested
      break;
    }
    case ExitRequest:
    {
std::cerr << "pendingExitDecisions: " << !systemState->pendingExitDecisions.empty() << std::endl;
      if ( auto token = getToken(systemState->pendingExitDecisions) ) {
        event = createExitEvent( systemState, token, vertex);
      }
      // no event is create if decision is not yet requested
      break;
    }
    case ChoiceRequest:
    {
      if ( auto token = getToken(systemState->pendingChoiceDecisions) ) {
        event = createChoiceEvent( systemState, token, vertex);
      }
      // no event is create if decision is not yet requested
      break;
    }
    case MessageDeliveryRequest:
    {
      if ( auto token = getToken(systemState->pendingMessageDeliveryDecisions) ) {
        event = createMessageDeliveryEvent( systemState, token, vertex );
      }
      // no event is create if decision is not yet requested
      break;
    }
    default:
    {
      throw std::logic_error("CPController: unsupported decision type");
    } 
  }
  if ( event ) {
    decisionQueue.pop();
  }
  return event;
}

CP::Solution& CPController::createSolution() {
  terminationEvent.reset();
  _solution = std::make_unique<CP::Solution>(model);
  
  // set collection evaluator
  _solution->setCollectionEvaluator( 
    [](double value) ->  std::expected< std::reference_wrapper<const std::vector<double> >, std::string >  {
      if ( value < 0 || value >= (double)collectionRegistry.size() ) {
        return std::unexpected("Unable to determine collection for index " + BPMNOS::to_string(value) );
      }
      return collectionRegistry[(size_t)value];
    }
  );

  // add evaluators for lookup tables
  for ( auto& lookupTable : scenario->model->lookupTables ) {
    _solution->addEvaluator( 
      lookupTable->name,
      [&lookupTable](const std::vector<double>& operands) -> double {
        return lookupTable->at(operands);
      }
    );
  }
  return *_solution;
}

const CP::Solution& CPController::getSolution() const {
  assert( _solution );
  return *_solution;
}

std::optional< BPMNOS::number > CPController::getTimestamp( const Vertex* vertex ) const {
  auto timestamp = _solution->evaluate( status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value );
  if ( timestamp ) {
    return (number)timestamp.value();
  }
  return std::nullopt;
}

void CPController::setTimestamp( const Vertex* vertex, BPMNOS::number timestamp ) {
  _solution->setVariableValue( status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value, (double)timestamp );
}


std::shared_ptr<Event> CPController::createEntryEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }

  return std::make_shared<EntryEvent>(token);
}

std::shared_ptr<Event> CPController::createExitEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }
  return std::make_shared<ExitEvent>(token);
}

std::shared_ptr<Event> CPController::createChoiceEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }

  auto& solution = getSolution();
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  BPMNOS::Values choices;
  for ( auto& choice : extensionElements->choices ) {
    assert( choice->attribute->category == BPMNOS::Model::Attribute::Category::STATUS );
    auto value = solution.getVariableValue( status.at(vertex)[choice->attribute->index].value );
    if ( !value ) {
      // no choice value provided
      return nullptr;
    }
    choices.push_back( (number)value.value() );
  }
  return std::make_shared<ChoiceEvent>(token,std::move(choices));
}

std::shared_ptr<Event> CPController::createMessageDeliveryEvent(const SystemState* systemState, Token* token, const Vertex* vertex) {
  auto timestamp = getTimestamp(vertex);
  if ( !timestamp.has_value() || systemState->getTime() < timestamp.value() ) {
    return nullptr;
  }

  auto& solution = getSolution();
  for ( auto& [participants,variable] : messageFlow ) {
    auto variableValue = solution.getVariableValue(variable);
    if ( participants.second != vertex || !variableValue || !variableValue.value() ) {
      // irrelevant message flow variable
      continue;
    }
    // find message with header indicating that the sender is the sending vertex
    for ( auto& [ message_ptr ] : systemState->outbox.at(participants.first->node->as<BPMN::FlowNode>()) ) {
      if ( auto message = message_ptr.lock();
        message &&
        message->header[ BPMNOS::Model::MessageDefinition::Index::Sender ] == participants.first->instanceId
      ) {
        return std::make_shared<MessageDeliveryEvent>(token, message.get());
      }
    }
  }
  // message is not yet sent
  return nullptr;
}
  
void CPController::createDecisionQueue() {
  auto& solution = getSolution();
  decisionQueue = std::queue< std::pair< RequestType, const Vertex* > >();
  
  // determine vertices sorted by sequence position
  std::vector<const Vertex *> sortedVertices( vertices.size() );

  auto& sequence = model.getSequences().front();
  assert( sequence.variables.size() == vertices.size() );
  for ( size_t i = 0; i < vertices.size(); i++) {
    auto position = solution.getVariableValue(sequence.variables[i]);
    assert( position );
std::cerr << "position[" << i << "] = " << position.value() << ": " << vertices[i]->reference() << std::endl;
    sortedVertices[ (size_t)position.value() - 1 ] = vertices[i];
  }
  
  // now insert into decision sequence
  for ( auto vertex : sortedVertices ) {
    
    if ( vertex->entry<BPMN::Activity>() ) {
      // enqueue entry decision
      decisionQueue.emplace(RequestType::EntryRequest, vertex);
      continue;
    }

    if ( vertex->exit<BPMN::MessageCatchEvent>() ) {
      // enqueue message delivery decision
      // ASSUMPTION: receive tasks are exited immediately after message is delivered
      decisionQueue.emplace(RequestType::MessageDeliveryRequest, vertex);
    }

    if ( vertex->exit<BPMNOS::Model::DecisionTask>() ) {
      // enqueue choice decision
      // ASSUMPTION: decision tasks are exited immediately after decision is made
      decisionQueue.emplace(RequestType::ChoiceRequest, vertex);
    }

    if ( vertex->exit<BPMN::Activity>() ) {
      // enqueue exit decision
      decisionQueue.emplace(RequestType::ExitRequest, vertex);
    }
  }
}

void CPController::createCP() {
  vertices.reserve( flattenedGraph.vertices.size() );
//std::cerr << "initializeVertices" << std::endl;
  // determine relevant vertices of all process instances
  for ( const Vertex& initialVertex : flattenedGraph.initialVertices ) {
    initializeVertices(&initialVertex);
  }

//std::cerr << "create sequence position variables" << std::endl;
  // create sequence position variables for all vertices
  auto& sequence = model.addSequence( "position", vertices.size() );
  for ( size_t i = 0; i < vertices.size(); i++ ) {
    position.emplace(vertices[i], sequence.variables[i]);
  } 

std::cerr << "createGlobalVariables" << std::endl;
  createGlobalVariables();

std::cerr << "createVertexVariables:" << flattenedGraph.vertices.size() << std::endl;
  // create vertex and message variables
  for ( auto vertex : vertices ) {
    createVertexVariables(vertex);
  }

std::cerr << "constrainGlobalVariables" << std::endl;
  constrainGlobalVariables();

std::cerr << "constrainDataVariables" << std::endl;
  for ( auto vertex : vertices ) {
    if ( vertex->entry<BPMN::Scope>() ) {
      constrainDataVariables(vertex);
    }
  }  

std::cerr << "constrainSequentialActivities" << std::endl;
  constrainSequentialActivities();
  
std::cerr << "createMessageVariables" << std::endl;
  createMessageVariables();
std::cerr << "Done" << std::endl;
std::cerr << model.stringify() << std::endl;  
}

void CPController::createGlobalVariables() {
  for ( auto attribute : scenario->model->attributeRegistry.globalAttributes ) {
    assert( attribute->index == globals.size() ); // ensure that the order of attributes is correct
    globals.emplace_back(
      model.addIndexedVariables(CP::Variable::Type::BOOLEAN, "defined_" + attribute->id ), 
      model.addIndexedVariables(CP::Variable::Type::REAL, "value_" + attribute->id) 
    );
    auto& [defined,value] = globals.back();
    // add variables holding initial values
    auto& initialValue = scenario->globals[attribute->index];
    if ( initialValue.has_value() ) {
      // defined initial value
      defined.emplace_back(true,true);
      value.emplace_back((double)initialValue.value(),(double)initialValue.value()); 
    }
    else {
      // undefined initial value
      defined.emplace_back(false,false);
      value.emplace_back(0.0,0.0); 
    }
    
    if ( attribute->isImmutable ) {
      // deduced variables
      for ( [[maybe_unused]] auto _ : flattenedGraph.globalModifiers ) {
        // use initial value for all data states
        defined.emplace_back(defined[0]);
        value.emplace_back(value[0]); 
      }
    }
    else {
      for ( [[maybe_unused]] auto _ : flattenedGraph.globalModifiers ) {
        // unconstrained variables for all data states
        defined.emplace_back();
        value.emplace_back(); 
      }
    }
  }
}

void CPController::createMessageVariables() {
  for ( auto recipient : messageRecipients ) {
    assert( recipient->exit<BPMN::MessageCatchEvent>() );
    CP::reference_vector<const CP::Variable> messages;
    for ( Vertex& sender : recipient->senders ) {
      assert( sender.exit<BPMN::MessageThrowEvent>() );
      // create binary decision variable for a message from sender to recipient
      messages.emplace_back( model.addBinaryVariable("message_{" + sender.reference() + " → " + recipient->reference() + "}" ) );
      const CP::Variable& message = messages.back();
      messageFlow.emplace( std::make_pair(&sender,recipient), message );
      
      model.addConstraint( message <= visit.at(recipient) );
      model.addConstraint( message <= visit.at(&sender) );
      
      model.addConstraint( message.implies( position.at(&sender) <= position.at(recipient) ) );

      model.addConstraint(
        // if a message is sent from a sender to a recipient, the recipient's timestamp must not 
        // be before the sender's timestamp
        message.implies (
          status.at(&sender)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          <= status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        )
      );

      /// ASSUMPTION: receive tasks are exited immediately after message is delivered
      if ( sender.node->represents<BPMN::SendTask>() ) {
        // if a message is sent from a send task to a recipient, the recipient's timestamp must 
        // be before the sender's exit timestamp
        if ( config.instantExit ) {
          model.addConstraint(
            message.implies (
              status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
              == status.at(exit(&sender))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
            )
          );
        }
        else {
          model.addConstraint(
            message.implies (
              status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
              <= status.at(exit(&sender))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
            )
          );
        }
      }

      // TODO: add message header constraints
    }
    CP::Expression sum;
    for ( const CP::Variable& message : messages ) {
      sum = sum + message;
    }
    // every recipient that is visited receives a message
    model.addConstraint( sum == visit.at(recipient) );
  }
}

void CPController::createMessageContent(const Vertex* vertex) {
  // TODO
  // every visited recipient must receive a message
  // message precedence constraints
  
  // message content
}

void CPController::createDataVariables(const FlattenedGraph::Vertex* vertex) {
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  for ( auto& attribute : extensionElements->data ) {
    IndexedAttributeVariables variables( 
      model.addIndexedVariables(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + "}," + attribute->id ),
      model.addIndexedVariables(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + "}," + attribute->id )
    );
    // add variables holding initial values
    auto given = scenario->getKnownValue(vertex->rootId, attribute.get(), 0);
    if ( given.has_value() ) {
      // defined initial value
      variables.defined.emplace_back(visit.at(vertex));
      variables.value.emplace_back(CP::if_then_else( visit.at(vertex), (double)given.value(), 0.0)); 
    }
    else if ( attribute->expression ) {
      // initial assignment
      assert( attribute->expression->type == Model::Expression::Type::ASSIGN ); 
      CP::Expression assignment = createExpression( vertex, *attribute->expression );
      variables.defined.emplace_back(visit.at(vertex));
      variables.value.emplace_back(CP::if_then_else( visit.at(vertex), assignment, 0.0)); 
    }
    else {
      // undefined initial value
      variables.defined.emplace_back(false,false);
      variables.value.emplace_back(0.0,0.0); 
    }
    
    assert( flattenedGraph.dataModifiers.contains(vertex) );
    if ( attribute->isImmutable ) {
      // deducible variables
      for ( [[maybe_unused]] auto _ : flattenedGraph.dataModifiers.at(vertex) ) {
        // use initial value for all data states
        variables.defined.emplace_back(variables.defined[0]);
        variables.value.emplace_back(variables.value[0]); 
      }
    }
    else {
      for ( [[maybe_unused]] auto _ : flattenedGraph.dataModifiers.at(vertex) ) {
        // unconstrained variables for all data states
        variables.defined.emplace_back();
        variables.value.emplace_back(); 
      }
    }
    data.emplace( std::make_pair(vertex, attribute.get()), std::move(variables) ); 
  }
}

void CPController::constrainDataVariables(const FlattenedGraph::Vertex* vertex) {
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( extensionElements->data.empty() ) return;
  
//std::cerr << "modifiers of " << vertex->reference() << std::endl;
  auto& dataModifiers = flattenedGraph.dataModifiers.at(vertex);
  for ( auto& [entry,exit] : dataModifiers ) {
//std::cerr << "modifier: " << entry.reference() << std::endl;
    auto& [localStatus,localData,localGlobals] = locals.at(&exit).back();
    for ( unsigned int i = 0; i < dataModifiers.size(); i++ ) {
      for ( auto& attribute : extensionElements->data ) {
        auto& index = dataIndex.at(&entry)[ entry.dataOwnerIndex(attribute.get()) ];
        // if data index at modifier entry equals i then the i+1-th data item equals the final data of the modifier
        model.addConstraint( 
          ( index == i ).implies( 
            data.at({vertex,attribute.get()}).defined[i + 1] == localData[attribute->index].defined
          ) 
        );
        model.addConstraint( 
          ( index == i ).implies( 
            data.at({vertex,attribute.get()}).value[i + 1] == localData[attribute->index].value
          ) 
        );
      }
    }
  }
}

void CPController::constrainGlobalVariables() {
  for ( auto& [entry,exit] : flattenedGraph.globalModifiers ) {
    auto& [localStatus,localData,localGlobals] = locals.at(&exit).back();
    for ( unsigned int i = 0; i < flattenedGraph.globalModifiers.size(); i++ ) {
      for ( auto attribute : scenario->model->attributeRegistry.globalAttributes ) {
        // if global index at modifier entry equals i then the i+1-th global item equals the final globals of the modifier
        model.addConstraint( 
          ( globalIndex.at(&entry) == i ).implies( 
            globals[attribute->index].defined[i + 1] == localGlobals[attribute->index].defined
          ) 
        );
        model.addConstraint( 
          ( globalIndex.at(&entry) == i ).implies( 
            globals[attribute->index].value[i + 1] == localGlobals[attribute->index].value
          ) 
        );
      }
    }
  }
}

void CPController::constrainSequentialActivities() {
  for ( auto& [performer,sequentialActivities] : flattenedGraph.sequentialActivities ) {
    for ( size_t i = 0; i + 1 < sequentialActivities.size(); i++ ) {
      for ( size_t j = i + 1; j < sequentialActivities.size(); j++ ) {
        auto& [entry1,exit1] = sequentialActivities[i];
        auto& [entry2,exit2] = sequentialActivities[j];
        model.addConstraint( 
          ( position.at(&entry1) < position.at(&entry2)).implies( position.at(&exit1) < position.at(&entry2) )
        );
      }
    }
  }
}

void CPController::createStatus(const Vertex* vertex) {
//std::cerr << "createStatus: " << vertex->reference() << std::endl;
  if ( vertex->type == Vertex::Type::ENTRY ) {
    createEntryStatus(vertex);
  }
  else {
    createExitStatus(vertex);
  }

  // TODO: sequential activity or multi-instance sequential activity
}

void CPController::createEntryStatus(const Vertex* vertex) {
std::cerr << "createEntryStatus: " << vertex->reference() << std::endl;
  status.emplace( vertex, std::vector<AttributeVariables>() );  
  auto& variables = status.at(vertex);

  if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
    loopCharacteristics.has_value() &&
    !flattenedGraph.dummies.contains(vertex)    
  ) {
    createLoopEntryStatus(vertex);
/*
    if ( loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
      createLoopEntryStatus(vertex);
//std::cerr << model.stringify() << std::endl;
    }
    else {
//      createMultiInstanceEntryStatus(vertex);
      assert(!"Not yet implemented");
    }
*/
    return;
  }
  
  if ( vertex->parent.has_value() ) {
    auto scope = vertex->parent.value().first.node;
    auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( vertex->entry<BPMN::UntypedStartEvent>() ) {
      // TODO: start-event
      assert( vertex->predecessors.size() == 1 ); // parent vertex is the only predecessors
      variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, status.at(&vertex->predecessors.front().get()) );
    }
    else if ( vertex->entry<BPMN::TypedStartEvent>() ) {
      // TODO: start-event
    }
    else if ( vertex->entry<BPMN::ExclusiveGateway>() && vertex->inflows.size() > 1 ) {
      std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives;
      for ( auto& [sequenceFlow,predecessor] : vertex->inflows ) {
        // add to alternatives
        alternatives.emplace_back( tokenFlow.at({&predecessor,vertex}), statusFlow.at({&predecessor,vertex}) );
      }
      variables = createAlternativeEntryStatus(vertex, extensionElements->attributeRegistry, std::move(alternatives));
    }
    else if ( vertex->entry<BPMN::FlowNode>() && vertex->inflows.size() > 1 ) {
      assert(vertex->entry<BPMN::ParallelGateway>() || vertex->entry<BPMN::InclusiveGateway>() );
      assert( vertex->predecessors.size() == 1 );
      if ( !vertex->entry<BPMN::ParallelGateway>() && !vertex->entry<BPMN::InclusiveGateway>() ) {
        throw std::runtime_error("CPController: illegal join at '" + vertex->node->id + "'");
      }
      std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs;
      for ( auto& [sequenceFlow,predecessor] : vertex->inflows ) {
        // add to alternatives
        inputs.emplace_back( tokenFlow.at({&predecessor,vertex}), statusFlow.at({&predecessor,vertex}) );
      }
      variables = createMergedStatus(vertex, extensionElements->attributeRegistry, std::move(inputs));
    }
    else if ( vertex->entry<BPMN::FlowNode>() ) {
      assert( vertex->inflows.size() == 1 );
      auto& [sequenceFlow,predecessor] = vertex->inflows.front();
      if ( sequenceFlow ) {   
        variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, statusFlow.at({&predecessor,vertex}) );
      }
      else {
        assert( vertex->node->represents<BPMN::Activity>() );
        assert( vertex->node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );
        variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, status.at(&predecessor) );
      }
/*
      auto& [sequenceFlow, predecessor] = vertex->inflows.front();
      if ( sequenceFlow ) {   
        // deduce visit from unique sequence flow
        auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , tokenFlow.at( std::make_pair(&predecessor, vertex) ) );
        visit.emplace(vertex, deducedVisit );
        visit.emplace(exit(vertex), deducedVisit );
      }
      else {
        // deduce visit from unique predecessor
        assert( vertex->node->represents<BPMN::Activity>() );
        assert( vertex->node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );

*/

    }
  }

  // add new attributes if necessary
  if ( auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
    variables.reserve(extensionElements->attributeRegistry.statusAttributes.size());
    for ( size_t i = variables.size(); i < extensionElements->attributeRegistry.statusAttributes.size(); i++) {
      auto attribute = extensionElements->attributeRegistry.statusAttributes[i];
std::cerr << attribute->id << std::endl;
      // add variables holding given values
      if ( auto given = scenario->getKnownValue(vertex->rootId, attribute, 0); given.has_value() ) {
        // defined initial value
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), (double)given.value(), 0.0)) 
        );
      }
      else if ( attribute->expression ) {
        // initial assignment
        assert( attribute->expression->type == Model::Expression::Type::ASSIGN ); 
        CP::Expression assignment = createExpression( vertex, *attribute->expression );
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), assignment, 0.0) )            
        );
      }
      else {
        // no given value
        bool defined = ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp );
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, 0.0, 0.0) 
        );
      }
    }
  }
}

void CPController::createExitStatus(const Vertex* vertex) {
std::cerr << "createExitStatus" << std::endl;
  auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();

  if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
    loopCharacteristics.has_value() &&
    flattenedGraph.dummies.contains(vertex)
  ) {
    if ( loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
      // use final loop exit status
      status.emplace( vertex, createLoopExitStatus(vertex) );
    }
    else {
      // merge multi-instance exit statuses
      std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs;
      for ( auto& [_,multiInstanceExit] : vertex->inflows ) {
        inputs.emplace_back( visit.at(&multiInstanceExit), status.at(&multiInstanceExit) );
      }
      status.emplace( vertex, createMergedStatus(vertex, extensionElements->attributeRegistry, std::move(inputs)) );
    }
    return;
  }

  if ( vertex->node->represents<BPMN::Scope>() ) {
    if ( vertex->inflows.empty() ) {
      throw std::runtime_error("CPController: empty scopes are not supported");
    }
    
    // scope has children
    auto getEndVertices = [&]() {
      std::vector< std::reference_wrapper<const Vertex> > endVertices;
      for ( auto& [_,candidate] : vertex->inflows ) {
        // ignore inflows of unreachable nodes
        if ( visit.contains(&candidate) ) {
          endVertices.push_back( candidate );
        }
      }
      if ( endVertices.empty() ) {
        throw std::runtime_error("CPController: unable to determine end nodes for  scope '" + vertex->node->id  + "'");
      }
      return endVertices;
    };
    std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs;
    for ( const Vertex& endVertex : getEndVertices() ) {
      assert(visit.contains(&endVertex));
      assert(status.contains(&endVertex));
      inputs.emplace_back( visit.at(&endVertex), status.at(&endVertex) );
    }

    assert(extensionElements);
    status.emplace( vertex, createMergedStatus(vertex, extensionElements->attributeRegistry, std::move(inputs)) );
    return;
  }

  // no scope or empty scope
//  assert( !vertex->exit<BPMN::Scope>() || vertex->inflows.size() == 1 );

std::cerr << vertex->reference() << std::endl;  
  const Vertex* entryVertex = entry(vertex);
  if ( !extensionElements ) {
    // token just runs through the node and exit status is the same as entry status
    extensionElements = vertex->parent.value().first.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
std::cerr << entryVertex->reference() << std::endl;  
    auto& entryStatus = status.at(entryVertex);
    std::vector<AttributeVariables> variables;
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
std::cerr << attribute->name << ": " << attribute->index << " == " <<  variables.size() << std::endl;
//      assert( attribute->index == variables.size() );
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].defined ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].value )
      );      
    }
    status.emplace( vertex, std::move(variables) );
    return;
  }

  if ( vertex->exit<BPMN::Task>() || vertex->exit<BPMN::MessageStartEvent>() ) {
    // create local attribute variables considering all changes made before the token leves the node
    createLocalAttributeVariables(vertex);
    // use final attribute values to deduce exit status
    auto& [ localStatus, localData, localGlobals ] = locals.at(vertex).back();

    // create exit status
    std::vector<AttributeVariables> variables;
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
      auto& [ defined, value ] = localStatus.at(attribute->index);
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        // exit timestamp may be later than deduced timestamp
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addRealVariable("value_{" + vertex->reference() + "}," + attribute->id )
        );
        auto& timestamp = variables.back().value;
        model.addConstraint( visit.at(vertex).implies( timestamp >= value ) );
      }
      else {
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
        );
      }      
    }
    status.emplace( vertex, std::move(variables) );
    return;
  }
std::cerr << "JOJO3" << std::endl;  
  // copy entry status references
  std::vector<AttributeVariables> currentStatus = status.at(entryVertex);
  //std::vector<AttributeVariables> currentData = xxx.at(entryVertex);
  //std::vector<AttributeVariables> currentGlobal = xxx.at(entryVertex);
  
  if ( vertex->node->represents<BPMNOS::Model::DecisionTask>() ) {
    // choices are represented by unconstrained status (data/global) attributes
    // TODO
      assert(!"Not yet implemented");
  }

  if ( vertex->node->represents<BPMN::MessageCatchEvent>() ) {
    // receive message content (globals and data attributes must not be changed)
    if ( extensionElements->messageDefinitions.size() == 1 ) {
      std::vector< std::tuple< std::string_view, size_t, AttributeVariables> > contentVariables;
      auto& messageDefinition = extensionElements->messageDefinitions.front();
      for (auto& [key,content] : messageDefinition->contentMap ) {
        auto attribute = content->attribute;
        
        // create variables for the message content (relevant constraints must be added separately)
        AttributeVariables attributeVariables = {
          model.addBinaryVariable("content[" + content->key + "]_defined_{" + vertex->reference() + "}," + attribute->id ), 
          model.addRealVariable("content[" + content->key + "]_value_{" + vertex->reference() + "}," + attribute->id )
        };

        // use message content variables in current status
        currentStatus.at(attribute->index) = attributeVariables;

        contentVariables.emplace_back( content->key, attribute->index, std::move(attributeVariables) );
      }

      messageContent.emplace( vertex, std::move(contentVariables) );
    }
    else if ( extensionElements->messageDefinitions.size() > 1 ) {
      // multi-instance receive task
      assert(!"Not yet supported");
    }
  }
  
  if ( vertex->node->represents<BPMN::MessageStartEvent>() ) {
    // add entry restrictions of event-subprocess
    // TODO
  }

  if ( vertex->node->represents<BPMN::Task>() ) {
    // apply operators
    // TODO
  }

  std::vector<AttributeVariables> variables;
  for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
    assert( attribute->index == variables.size() );
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, currentStatus[attribute->index].defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, currentStatus[attribute->index].value )
    );      

    // ensure that attribute only has a value if vertex is visited and the attribute ifis defined
    assert( visit.contains(vertex) );
    model.addConstraint( variables[attribute->index].defined <= visit.at(vertex) );
    model.addConstraint( (!variables[attribute->index].defined).implies( variables[attribute->index].value == 0.0 ) );
  }

  status.emplace( vertex, std::move(variables) );
}

std::pair< CP::Expression, CP::Expression > CPController::getLocalAttributeVariables( const Model::Attribute* attribute, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables ) {
  auto& [ status, data, globals ] = localVariables;
  if ( attribute->category == Model::Attribute::Category::STATUS ) {
    return std::make_pair<CP::Expression,CP::Expression>( 
      status[attribute->index].defined,
      status[attribute->index].value 
    );
  }
  else if ( attribute->category == Model::Attribute::Category::DATA ) {
    return std::make_pair<CP::Expression,CP::Expression>( 
      data[attribute->index].defined,
      data[attribute->index].value 
    );
  }
  else {
    assert( attribute->category == Model::Attribute::Category::GLOBAL );
    return std::make_pair<CP::Expression,CP::Expression>( 
      globals[attribute->index].defined,
      globals[attribute->index].value 
    );
  }
}

CP::Expression CPController::createOperatorExpression( const Model::Expression& operator_, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables ) {
  auto compiled = LIMEX::Expression<CP::Expression,CP::Expression>(operator_.expression, limexHandle);
  
  std::vector<CP::Expression> variables;
  for ( auto& variableName : compiled.getVariables() ) {
    auto attribute = operator_.attributeRegistry[variableName];
    if( attribute->type == ValueType::COLLECTION ) {
      throw std::runtime_error("CPController: illegal operator expression '" + operator_.expression + "'");
    }

    auto [defined,value] = getLocalAttributeVariables(attribute,localVariables);
    variables.push_back( value );
  }
  
  std::vector<CP::Expression> collectionVariables;
  for ( auto& variableName : compiled.getCollections() ) {
    auto attribute = operator_.attributeRegistry[variableName];
    if( attribute->type != ValueType::COLLECTION ) {
      throw std::runtime_error("CPController: illegal operator expression '" + operator_.expression + "'");
    }

    auto [defined,value] = getLocalAttributeVariables(attribute,localVariables);
    collectionVariables.push_back( value );
  }
  
  return compiled.evaluate(variables,collectionVariables);
}

void CPController::createLocalAttributeVariables(const Vertex* vertex) {
  assert( vertex->exit<BPMN::Task>() || vertex->exit<BPMN::MessageStartEvent>() );

  auto  extensionElements = 
    vertex->exit<BPMN::Task>() ?
    vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>() :
    vertex->parent.value().first.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  ;

  if ( extensionElements->messageDefinitions.size() > 1 ) {
    assert(!"Not yet implemented");
  }

  auto messageDefinition = extensionElements->messageDefinitions.size() == 1 ? extensionElements->messageDefinitions[0].get() : nullptr;

  auto localAttributeVariables = [&](const auto& attributes) -> std::vector<AttributeVariables> {
    std::vector<AttributeVariables> variables;

    auto findChoice = [extensionElements](BPMNOS::Model::Attribute* attribute) -> BPMNOS::Model::Choice* {
      auto it = std::find_if(
        extensionElements->choices.begin(),
        extensionElements->choices.end(),
        [&](const auto& choice) {
          return choice->attribute == attribute;
        }
      ); 
      return ( it != extensionElements->choices.end() ? it->get() : nullptr );
    };

    auto findContent = [messageDefinition](BPMNOS::Model::Attribute* attribute) -> BPMNOS::Model::Content* {
      if ( !messageDefinition ) return nullptr;
      auto it = std::find_if(
        messageDefinition->contentMap.begin(),
        messageDefinition->contentMap.end(),
        [&](const auto& mapEntry) {
          auto& [key, content] = mapEntry;
          return content->attribute == attribute;
        }
      ); 
      return ( it != messageDefinition->contentMap.end() ? it->second.get() : nullptr );
    };

    auto initialVariables = [&](BPMNOS::Model::Attribute* attribute) -> std::pair<CP::Expression,CP::Expression> {
      if ( attribute->category == BPMNOS::Model::Attribute::Category::STATUS ) {
        return std::make_pair<CP::Expression,CP::Expression>( 
          status.at(entry(vertex))[attribute->index].defined, 
          status.at(entry(vertex))[attribute->index].value
        );
      }
      else if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        auto& index = dataIndex.at(entry(vertex))[ entry(vertex)->dataOwnerIndex(attribute) ];
        return std::make_pair<CP::Expression,CP::Expression>( 
          data.at( {&vertex->dataOwner(attribute).first, attribute } ).defined[ index ], 
          data.at( {&vertex->dataOwner(attribute).first, attribute } ).value[ index ]
        );
      }
      else {
        assert( attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL );
        auto& index = globalIndex.at(vertex);
        return std::make_pair<CP::Expression,CP::Expression>( 
          globals[attribute->index].defined[ index ],
          globals[attribute->index].value[ index ] 
        );
      }
    };

    for ( auto attribute : attributes ) {
      auto [defined,value] = initialVariables(attribute);
    
      if ( auto choice = findChoice(attribute) ) {
        if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
        ) {
          throw std::runtime_error("CPController: timestamp must not be a choice");
        }

        // attribute set by choice
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, visit.at(vertex) ), 
          model.addRealVariable("value_{" + vertex->shortReference() + ",0}," + attribute->id )
        );
//        auto& variable = variables.back().value;
        // TODO: 
        // create constraints limiting the choice
        // timestamp represents the time the choice is made
        assert(!"Not yet implemented");
      }
      else if ( auto content = findContent(attribute) ) {
        if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
        ) {
          throw std::runtime_error("CPController: timestamp must not be changed by message content");
        }
        // TODO: 
        // deduce attribute value from message content
        // timestamp represents the time of the message delivery
        assert(!"Not yet implemented");
      }
      else {
        if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp &&
          ( vertex->exit<BPMN::ReceiveTask>() || vertex->exit<BPMN::SendTask>() || vertex->exit<BPMNOS::Model::DecisionTask>())
        ) {
          if ( vertex->exit<BPMN::ReceiveTask>() ) {
            // TODO: deduce timestamp from time of message delivery
            assert(!"Not yet implemented");
          }
          else if ( vertex->exit<BPMN::SendTask>() ) {
            // TODO: deduce timestamp from time of message delivery
            assert(!"Not yet implemented");
          }
          else {
            assert( vertex->exit<BPMNOS::Model::DecisionTask>() );
            // TODO: deduce timestamp from time of choice
            assert(!"Not yet implemented");
          }
        }
        else {
          // all local attribute variables are deduced from initial variables upon entry
          variables.emplace_back(
            model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, defined ), 
            model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + ",0}," + attribute->id, value )
          );
        }
      }
    }
    
    return variables;
  };
  
  locals.emplace(
    vertex, 
    std::vector{ 
      std::make_tuple(
        localAttributeVariables(extensionElements->attributeRegistry.statusAttributes),
        localAttributeVariables(extensionElements->attributeRegistry.dataAttributes),
        localAttributeVariables(extensionElements->attributeRegistry.globalAttributes)
      )
    }
  );

  auto& current = locals.at(vertex);
  for ( auto& operator_ : extensionElements->operators ) {
    auto& [ currentStatus, currentData, currentGlobals ] = current.back();

    auto updatedLocalAttributeVariables = [&](const auto& attributes, std::vector<AttributeVariables>& attributeVariables ) -> std::vector<AttributeVariables> {
      std::vector<AttributeVariables> variables;
      for ( auto attribute : attributes ) {
        if ( attribute == operator_->attribute ) {
          // determine updated variable value (only change value if vertex is visited)
          if ( operator_->expression.type == Model::Expression::Type::UNASSIGN ) {
            variables.emplace_back(
              model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, CP::if_then_else( visit.at(vertex), false, attributeVariables[attribute->index].defined) ), 
              model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, CP::if_then_else( visit.at(vertex), 0.0, attributeVariables[attribute->index].value) )
            );            
          }
          else if ( operator_->expression.type == Model::Expression::Type::ASSIGN ) {        
            CP::Expression value = createOperatorExpression( operator_->expression, current.back() );
            variables.emplace_back(
              model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, CP::if_then_else( visit.at(vertex), true, attributeVariables[attribute->index].defined) ), 
              model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, CP::if_then_else( visit.at(vertex), value, attributeVariables[attribute->index].value) )            
            );
            // TODO: add constraints ensuring that all inputs used by expression are defined
          }
          else {
            throw std::runtime_error("CPController: illegal operator expression: " + operator_->expression.expression );
          }
        }
        else {
          // variable value remains unchanged
          variables.emplace_back(
            model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, attributeVariables[attribute->index].defined ), 
            model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + "," + std::to_string(current.size()) + "}," + attribute->id, attributeVariables[attribute->index].value )            
          );
        }
      }
      return variables;
    };
    
    
    current.emplace_back(
      updatedLocalAttributeVariables(extensionElements->attributeRegistry.statusAttributes,currentStatus),
      updatedLocalAttributeVariables(extensionElements->attributeRegistry.dataAttributes,currentData),
      updatedLocalAttributeVariables(extensionElements->attributeRegistry.globalAttributes,currentGlobals)
    );
  } 
}

std::vector<CPController::AttributeVariables> CPController::createUniquelyDeducedEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector<AttributeVariables>& inheritedStatus) {
  assert( vertex->type == Vertex::Type::ENTRY );
  assert( !vertex->node->represents<BPMN::Process>() );

  std::vector<AttributeVariables> variables;
  variables.reserve( attributeRegistry.statusAttributes.size() );
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    assert( variables.size() == attribute->index );
    auto& [ defined, value ] = inheritedStatus.at(attribute->index);
    if ( auto activity = vertex->node->represents<BPMN::Activity>();
      activity &&
      !activity->loopCharacteristics.has_value() &&
      attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp 
    ) {
      // entry of simple activity may be later than the deduced timestamp ( entry of loop or multi-instance actvities is deduced)
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
        model.addRealVariable("value_{" + vertex->reference() + "}," + attribute->id )
      );
      auto& timestamp = variables.back().value;
      model.addConstraint( visit.at(vertex).implies( timestamp >= value ) );
    }
    else {
      // deduce variable
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
      );
    }
  }

  return variables;
}


std::vector<CPController::AttributeVariables> CPController::createAlternativeEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives) {
  assert( vertex->type == Vertex::Type::ENTRY );
  assert( !vertex->node->represents<BPMN::Process>() );
  assert( !vertex->node->represents<BPMN::Activity>() );
  std::vector<AttributeVariables> variables;
  variables.reserve( attributeRegistry.statusAttributes.size() );
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    // deduce variable
    CP::Expression defined(false);
    CP::Expression value(0.0);
    for ( auto& [ active, attributeVariables] : alternatives ) {
      assert( attributeVariables.size() == attributeRegistry.statusAttributes.size() );
      defined = defined || attributeVariables[attribute->index].defined;
      value = value + attributeVariables[attribute->index].value;
    }
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
    );
  }

  return variables;
}


std::vector<CPController::AttributeVariables> CPController::createMergedStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs) {
//  assert( ( vertex->type == Vertex::Type::ENTRY && vertex->inflows.size() > 1) || vertex->exit<BPMN::Scope>() );
//  assert( !vertex->entry<BPMN::Activity>() );
  std::vector<AttributeVariables> variables;
  variables.reserve( attributeRegistry.statusAttributes.size() );
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      std::vector<CP::Expression> terms;
      assert( !inputs.empty() );
      for ( auto& [ active, attributeVariables] : inputs ) {
        terms.emplace_back( attributeVariables[attribute->index].value );
      }
      // entry of activity is the same as the maximum timestamp
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), CP::max(terms), 0.0 ) )
      );
    }
    else {
      // deduce variable

      // determine whether merged variable is defined
      CP::Expression exists(false);
      for ( auto& [ _, attributeVariables] : inputs ) {
        // check whether there is at least one input is defined 
        exists = exists || attributeVariables[attribute->index].defined;
      }
      CP::Expression allSame(true);
      // check whether all defined inputs have the same value
      for ( size_t i = 0; i + 1 < inputs.size(); i++ ) {
        for ( size_t j = i + 1; j < inputs.size(); j++ ) {
          auto& someVariables = inputs[i].second;
          auto& otherVariables = inputs[j].second;
          allSame = allSame && CP::if_then_else( someVariables[attribute->index].defined && otherVariables[attribute->index].defined, someVariables[attribute->index].value == otherVariables[attribute->index].value, true);
        }
      }
      auto& defined = model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, exists && allSame);

      // determine merged variable value
      CP::Cases cases;
      for ( auto& [ active, attributeVariables] : inputs ) {
        cases.emplace_back( attributeVariables[attribute->index].defined, attributeVariables[attribute->index].value );
      }
      auto& value = model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( defined, CP::n_ary_if( cases, 0.0 ), 0.0) );
      
      variables.emplace_back( defined, value );
    }
  }
  return variables;
}

CP::Expression CPController::getLoopIndex(const Vertex* vertex) {
  auto predecessor = &vertex->inflows.front().second;
  if ( getLoopCharacteristics(vertex).value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    const BPMNOS::Model::Attribute* attribute = 
      extensionElements->loopIndex.has_value() ? 
      extensionElements->loopIndex.value()->expression->isAttribute() :
      nullptr
    ;
    auto& [ defined, value ] = status.at(predecessor).at(attribute->index);
    return value + 1;
  }
  else {
    size_t i = 0; // determine index of activity
    for ( ; i < predecessor->outflows.size(); i++ ) {
      if ( vertex == &predecessor->outflows[i].second ) {
        break;
      }
    }
    return (double)i+1;
  }
}

void CPController::createLoopEntryStatus(const Vertex* vertex) {
std::cerr << "createLoopEntryStatus" << std::endl;
  assert( vertex->inflows.size() == 1 );
  assert( status.contains(vertex) );
  auto& variables = status.at(vertex);
  auto& predecessor = vertex->inflows.front().second;
  auto& priorStatus = status.at(&predecessor);
std::cerr << predecessor.reference() << "/" << priorStatus.size() << std::endl;
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  const BPMNOS::Model::Attribute* loopIndex = 
    extensionElements->loopIndex.has_value() ? 
    extensionElements->loopIndex.value()->expression->isAttribute() :
    nullptr
  ;

  // deduce status for loop visit 
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  
  for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
    assert( variables.size() == attribute->index );
std::cerr << attribute->id << std::endl;
    auto& [ defined, value ] = priorStatus.at(attribute->index);
    if ( vertex->node->represents<BPMN::Activity>() && attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      // entry of activity may be later than the deduced timestamp
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
        model.addRealVariable("value_{" + vertex->reference() + "}," + attribute->id )
      );
      auto& timestamp = variables.back().value;
      model.addConstraint( visit.at(vertex).implies( timestamp >= value ) );
    }
    else if ( attribute == loopIndex ) {
      // set or increment loop index (initial value is assumed to be undefined, and therefore has a value of zero)
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
        model.addVariable(CP::Variable::Type::REAL,"value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), getLoopIndex(vertex), 0 ) )
      );
//std::cerr << model.stringify() << std::endl;
//std::cerr << variables.back().value.stringify() << std::endl;
//    assert(!"Not yet implemented");
    }
    else if ( attribute->expression ) {
      // initial assignment
      assert( attribute->expression->type == Model::Expression::Type::ASSIGN ); 
      CP::Expression assignment = createExpression( vertex, *attribute->expression );
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), assignment, 0.0) )            
      );
    }
    else {
      // deduce variable
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), defined, false ) ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), value, 0.0 ) )
      );
    }
  }
}

std::vector<CPController::AttributeVariables> CPController::createLoopExitStatus(const Vertex* vertex) {
  assert( vertex->inflows.size() >= 1 );
//  auto& priorStatus = status.at(&predecessor);
  // deduce exit status for loop vertex 
  std::vector<AttributeVariables> variables;
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  
  for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
    CP::Expression defined(false);
    CP::Expression value(0.0);
    for ( size_t i = 0; i < vertex->inflows.size(); i++ ) {
      auto& predecessor = vertex->inflows[i].second;
      // determine condition whether predecessor is the last visited loop vertex
      CP::Expression condition = ( i + 1 < vertex->inflows.size() ) ? visit.at(&predecessor) && !visit.at(&vertex->inflows[i+1].second) : visit.at(&predecessor);
      
      auto &priorStatus = status.at(&predecessor);
      defined = defined || ( condition * priorStatus.at(attribute->index).defined ); 
      value = value + ( condition * priorStatus.at(attribute->index).value ); 
    }
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
    );
  }
  return variables;
}


std::vector< const FlattenedGraph::Vertex* > CPController::getReachableVertices(const FlattenedGraph::Vertex* initialVertex) {
  std::vector< const FlattenedGraph::Vertex* > reachableVertices;
  std::unordered_map<const FlattenedGraph::Vertex*, size_t> inDegree;
  
  std::deque<const FlattenedGraph::Vertex*> queue;
  queue.push_back(initialVertex);
    
  // determine vertices in topological order
  while ( !queue.empty() ) {
//std::cerr << "Queue " << queue.size() << std::endl;
    const FlattenedGraph::Vertex* current = queue.front();
    queue.pop_front();
    reachableVertices.push_back(current);
    inDegree.erase(current);
    
    for ( auto& [_,vertex] : current->outflows ) {
      if ( !inDegree.contains(&vertex) ) {
        // initialize in degree
        inDegree[&vertex] = vertex.inflows.size() + vertex.predecessors.size();
      }
      // decrement in degree and add vertex to queue if zero
      if ( --inDegree.at(&vertex) == 0 ) {
        queue.push_back(&vertex);
      }
    }
    for ( const FlattenedGraph::Vertex& vertex : current->successors ) {
      if ( !inDegree.contains(&vertex) ) {
        // initialize in degree
        inDegree[&vertex] = vertex.inflows.size() + vertex.predecessors.size();
      }
//std::cerr << "Vertex " << vertex.reference() << " has inDegree " << inDegree.at(&vertex) << "/" << vertex.inflows.size() << "/" << vertex.predecessors.size() << std::endl;
      // decrement in degree and add vertex to queue if zero
      if ( --inDegree.at(&vertex) == 0 ) {
        queue.push_back(&vertex);
      }
    }
  }
  
  if ( inDegree.size() ) {
    throw std::runtime_error("CPController: cycle detected in '" + BPMNOS::to_string(initialVertex->rootId, STRING) + "'");
  }
//std::cerr << "reachableVertices:" << reachableVertices.size() << std::endl;
  return reachableVertices;
}

void CPController::initializeVertices(const FlattenedGraph::Vertex* initialVertex) {
  for ( const Vertex* vertex : getReachableVertices(initialVertex) ) {
    vertices.push_back(vertex);
  }
}

void CPController::createVertexVariables(const FlattenedGraph::Vertex* vertex) {
std::cerr << "createVertexVariables: " << vertex->reference() << std::endl;
  // make vertex retrievable from token information
//  vertexMap[std::make_tuple(vertex->node,vertex->instanceId,vertex->type)] = vertex; // TODO: remove
  
std::cerr << "createGlobalIndexVariable" << std::endl;
  createGlobalIndexVariable(vertex);
std::cerr << "createDataIndexVariables" << std::endl;
  createDataIndexVariables(vertex);
  
  if ( vertex->type == Vertex::Type::ENTRY ) {
std::cerr << "createEntryVariables" << std::endl;
    createEntryVariables(vertex);
  }

std::cerr << "createDataVariables: " << vertex->reference() << std::endl;
  if ( vertex->entry<BPMN::Scope>() ) {
    createDataVariables(vertex);
  }

std::cerr << "createStatus" << std::endl;
  createStatus(vertex);

  if ( vertex->type == Vertex::Type::EXIT ) {
    createExitVariables(vertex);
  }
    
std::cerr << "createSequenceConstraints" << std::endl;
  createSequenceConstraints(vertex);

std::cerr << "createRestrictions" << std::endl;
  createRestrictions(vertex);

std::cerr << "Done(createVertexVariables)" << std::endl;
}

std::pair< CP::Expression, CP::Expression > CPController::getAttributeVariables( const Vertex* vertex, const Model::Attribute* attribute) {
  if ( attribute->category == Model::Attribute::Category::STATUS ) {
    assert( status.contains(vertex) );
    assert( attribute->index < status.at(vertex).size() );
    return std::make_pair<CP::Expression,CP::Expression>( 
      status.at(vertex)[attribute->index].defined,
      status.at(vertex)[attribute->index].value 
    );
  }
  else if ( attribute->category == Model::Attribute::Category::DATA ) {
    assert( data.contains( {&vertex->dataOwner(attribute).first, attribute } ) );
    auto& index = dataIndex.at(vertex)[ vertex->dataOwnerIndex(attribute) ];
    return std::make_pair<CP::Expression,CP::Expression>( 
      data.at( {&vertex->dataOwner(attribute).first, attribute } ).defined[ index ],
      data.at( {&vertex->dataOwner(attribute).first, attribute } ).value[ index ]
    );
  }
  else {
    assert( attribute->category == Model::Attribute::Category::GLOBAL );
    assert( globals.size() > attribute->index );
    auto& index = globalIndex.at(vertex);
    return std::make_pair<CP::Expression,CP::Expression>( 
      globals[attribute->index].defined[ index ],
      globals[attribute->index].value[ index ] 
    );
  }
}

void CPController::createEntryVariables(const FlattenedGraph::Vertex* vertex) {
std::cerr << "HUHU1" << std::endl;      
  // visit variable
  if ( vertex->node->represents<BPMN::UntypedStartEvent>() ) {
    assert( vertex->inflows.size() == 1 );
    assert( vertex->predecessors.size() == 1 );
    assert( vertex->senders.empty() );
    
    // deduce visit from parent
    auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}", visit.at( &vertex->parent.value().first ) );
    visit.emplace(vertex, deducedVisit );
    visit.emplace(exit(vertex), deducedVisit );
  }
  else if ( vertex->node->represents<BPMN::TypedStartEvent>() ) {
    assert(!"Not yet implemented");
  }
  else if ( vertex->node->represents<BPMN::FlowNode>() ) {
std::cerr << vertex->reference() << ": " <<  vertex->loopIndices.size()  << "/" << flattenedGraph.loopIndexAttributes.at(vertex->node).size() << std::endl;      
    if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
      loopCharacteristics.has_value() &&
      !flattenedGraph.dummies.contains(vertex)    
    ) {
      if ( loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
        // loop vertex
        auto predecessor = &vertex->inflows.front().second;
        if ( flattenedGraph.dummies.contains(predecessor) ) {
          // first loop vertex
          auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) );
          visit.emplace(vertex, deducedVisit );
          visit.emplace(exit(vertex), deducedVisit );
std::cerr << deducedVisit.stringify() << std::endl;
        }
        else {
          auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
          if ( extensionElements->loopCondition.has_value() ) {
            // vertex is visited only if predecessor is visited and loop condition is satisifed
            auto condition = createExpression(predecessor, *extensionElements->loopCondition.value()->expression);
            auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) && condition );
            visit.emplace(vertex, deducedVisit );
            visit.emplace(exit(vertex), deducedVisit );
std::cerr << deducedVisit.stringify() << std::endl;
          }
          else {
            // no condition is given, vertex is visited if predecessor is visited
            auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) );
            visit.emplace(vertex, deducedVisit );
            visit.emplace(exit(vertex), deducedVisit );
          }
        }
      }
      else {
        // multi-instance vertex
        auto predecessor = &vertex->inflows.front().second;
        auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
        assert( extensionElements->loopCardinality.has_value() );
        CP::Expression cardinality = createExpression( predecessor, *extensionElements->loopCardinality.value()->expression );
        auto index = getLoopIndex(vertex);
        auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) && ( index <= cardinality ) );
        visit.emplace(vertex, deducedVisit );
        visit.emplace(exit(vertex), deducedVisit );
//std::cerr << deducedVisit.stringify() << std::endl;
//        assert(!"Not yet implemented");
      }
    }
    else if ( vertex->inflows.size() == 1 ) {
std::cerr << "HUHU3" << std::endl;
      auto& [sequenceFlow, predecessor] = vertex->inflows.front();
      if ( sequenceFlow ) {   
        // deduce visit from unique sequence flow
        auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , tokenFlow.at( std::make_pair(&predecessor, vertex) ) );
        visit.emplace(vertex, deducedVisit );
        visit.emplace(exit(vertex), deducedVisit );
      }
      else {
        // deduce visit from unique predecessor
        assert( vertex->node->represents<BPMN::Activity>() );
        assert( vertex->node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );

        auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( &predecessor ) );
        visit.emplace(vertex, deducedVisit );
        visit.emplace(exit(vertex), deducedVisit );
//        assert(!"Not yet implemented");
      }
    }
/*
    else if (
      vertex->node->represents<BPMN::Activity>() && 
      vertex->node->as<BPMN::Activity>()->loopCharacteristics.has_value()
    ) {
      assert( vertex->node->as<BPMN::Activity>()->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::MultiInstanceParallel );
      assert(!"Not yet implemented");
    }
*/
    else {
std::cerr << vertex->jsonify().dump() << std::endl;
      assert(!"Not yet implemented");
    }
  }
  else if ( vertex->node->represents<BPMN::Process>() ) {
    // every process vertex is visited
    auto& knownVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}", true, true );
    visit.emplace(vertex, knownVisit );
    visit.emplace(exit(vertex), knownVisit );
  }

std::cerr << "HUHU4" << std::endl;      
}

void CPController::createExitVariables(const Vertex* vertex) {
std::cerr << "createExitVariables" << std::endl;
  if ( vertex->node->represents<BPMN::FlowNode>() ) {
    if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
      loopCharacteristics.has_value() &&
      loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard
    ) {
      if ( vertex->loopIndices.size() == flattenedGraph.loopIndexAttributes.at(vertex->node).size() ) {
/*      assert( flattenedGraph.vertexMap.contains(vertex->node) );
      assert( flattenedGraph.vertexMap.at(vertex->node).contains(vertex->instanceId) );
      auto& siblings = flattenedGraph.vertexMap.at(vertex->node).at(vertex->instanceId);
      if ( vertex != &siblings.back().get() ) {
*/
        return;
      }
    }

    // flow variables
    if ( vertex->outflows.size() == 1 ) {
      tokenFlow.emplace( 
        std::make_pair(vertex,&vertex->outflows.front().second), 
        model.addVariable(CP::Variable::Type::BOOLEAN, "tokenflow_{" + vertex->reference() + " → " + vertex->outflows.front().second.reference() + "}", visit.at(vertex) ) 
      );
      auto extensionElements = vertex->parent.value().first.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      std::vector<AttributeVariables> variables;
      variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
      for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
//std::cerr << "statusAttribute: " << name << "/" << vertex->reference() << std::endl;
        assert( tokenFlow.contains({vertex,&vertex->outflows.front().second}) );
        assert( status.contains(vertex) );
        // deduce variable
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "statusflow_defined_{" + vertex->reference() + " → " + vertex->outflows.front().second.reference() + "}," + attribute->id, 
            CP::if_then_else( 
              tokenFlow.at({vertex,&vertex->outflows.front().second}), 
              status.at(vertex)[attribute->index].defined, 
              false
            )
          ), 
          model.addVariable(CP::Variable::Type::REAL, "statusflow_value_{" + vertex->reference() + " → " + vertex->outflows.front().second.reference() + "}," + attribute->id, 
            CP::if_then_else( 
              tokenFlow.at({vertex,&vertex->outflows.front().second}), 
              status.at(vertex)[attribute->index].value, 
              0.0
            )
          )
        );
      }      

      statusFlow.emplace( 
        std::make_pair(vertex,&vertex->outflows.front().second), 
        std::move(variables)
      );
    }
    else if ( vertex->outflows.size() > 1 ) {
std::cerr << vertex->reference() << std::endl;
      assert(!"Not yet implemented");
    }
  }
std::cerr << "Done(exitvariables)" << std::endl;
}

void CPController::createSequenceConstraints(const Vertex* vertex) {
  auto addConstraints = [&](const Vertex* predecessor) {
    assert( position.contains(predecessor) );
    assert( position.contains(vertex) );
    assert( visit.contains(vertex) );
    assert( status.contains(predecessor) );
    assert( status.at(predecessor).size() >= BPMNOS::Model::ExtensionElements::Index::Timestamp );
    assert( status.contains(vertex) );
    assert( status.at(vertex).size() >= BPMNOS::Model::ExtensionElements::Index::Timestamp );

    model.addConstraint( position.at(predecessor) + 1 <= position.at(vertex) );
/*
std::cerr << predecessor->reference() << " before " << vertex->reference()  << std::endl;  
std::cerr << visit.at(vertex).stringify() << "\n implies \n";
std::cerr << status.at(predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value.stringify() << "\n <= \n";
std::cerr << status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value.stringify() << std::endl;
*/    
    model.addConstraint(
      // if a vertex is visited, its timestamp must not be before the predecessors timestamp
      // as all timestamps are non-negative and zero if not visited, no condition on a visit
      // of the predecessor is required
      visit.at(vertex).implies(
        status.at(predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        <= status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
      )
    );
//std::cerr << model.getConstraints().back().stringify() << std::endl;
  };

  for ( auto& [sequenceFlow, predecessor] : vertex->inflows ) {
    addConstraints(&predecessor);
  }

  for ( const Vertex& predecessor : vertex->predecessors ) {
    addConstraints(&predecessor);
  }
}

void CPController::createRestrictions(const Vertex* vertex) {
  auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  if ( !extensionElements ) return;
  
  for ( auto& restriction : extensionElements->restrictions ) {
    if ( 
      ( vertex->type == Vertex::Type::ENTRY && restriction->scope != Model::Restriction::Scope::EXIT ) ||
      ( vertex->type == Vertex::Type::EXIT && restriction->scope != Model::Restriction::Scope::ENTRY )
    ) {
      // create constraints
      if ( restriction->expression.type == Model::Expression::Type::IS_NULL ) {
        assert( restriction->expression.variables.size() == 1 );
        auto attribute = restriction->expression.variables.front();
        auto [defined,value] = getAttributeVariables(vertex,attribute);
        model.addConstraint( visit.at(vertex).implies(defined == false) );
      }
      else if ( restriction->expression.type == Model::Expression::Type::IS_NOT_NULL ) {
        assert( restriction->expression.variables.size() == 1 );
        auto attribute = restriction->expression.variables.front();       
        auto [defined,value] = getAttributeVariables(vertex,attribute);
        model.addConstraint( visit.at(vertex).implies(defined == true) );
      }
      else {
        assert( restriction->expression.type == Model::Expression::Type::OTHER );
        for ( auto attribute : restriction->expression.variables ) {
          // all variables in expression must be defined
          auto [defined,value] = getAttributeVariables(vertex,attribute);
          model.addConstraint( visit.at(vertex).implies(defined == true) );
        }      
        // add restriction
        auto constraint = createExpression( vertex, restriction->expression );
        model.addConstraint( visit.at(vertex).implies(constraint) );
      }
    }
  }
}

// TODO: replace
CP::Expression CPController::createExpression(const Vertex* vertex, const Model::Expression& expression) {
  assert( vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  auto compiled = LIMEX::Expression<CP::Expression,CP::Expression>(expression.expression, limexHandle);
  
  std::vector<CP::Expression> variables;
  for ( auto& variableName : compiled.getVariables() ) {
    auto attribute = extensionElements->attributeRegistry[variableName];
    if( attribute->type == ValueType::COLLECTION ) {
      throw std::runtime_error("CPController: illegal expression '" + expression.expression + "'");
    }

    auto [defined,value] = getAttributeVariables(vertex,attribute);
std::cerr << value.stringify() << std::endl;
    variables.push_back( value );
  }
  
  std::vector<CP::Expression> collectionVariables;
  for ( auto& variableName : compiled.getCollections() ) {
    auto attribute = extensionElements->attributeRegistry[variableName];
    if( attribute->type != ValueType::COLLECTION ) {
      throw std::runtime_error("CPController: illegal expression '" + expression.expression + "'");
    }

    auto [defined,value] = getAttributeVariables(vertex,attribute);
std::cerr << value.stringify() << std::endl;
    collectionVariables.push_back( value );
  }

  return compiled.evaluate(variables,collectionVariables);
}

void CPController::createGlobalIndexVariable(const Vertex* vertex) {
  CP::Expression index;
  for ( auto& [modifierEntry, modifierExit] : flattenedGraph.globalModifiers ) {
    // create auxiliary variables indicating modifiers preceding the vertex
    index = index + model.addVariable(CP::Variable::Type::BOOLEAN, "precedes_{" + modifierExit.reference() + " → " + vertex->reference() + "}", position.at(&modifierExit) <= position.at(vertex) );
  }  
  globalIndex.emplace( vertex, model.addVariable(CP::Variable::Type::INTEGER, "globals_index_{" + vertex->reference() + "}", index ) );
}


void CPController::createDataIndexVariables(const Vertex* vertex) {
  CP::reference_vector< const CP::Variable > dataIndices;
  dataIndices.reserve( vertex->dataOwners.size() );
  for ( Vertex& dataOwner : vertex->dataOwners ) {
    assert( flattenedGraph.dataModifiers.contains(&dataOwner) ); 
  
    CP::Expression index;
    for ( auto& [modifierEntry, modifierExit] : flattenedGraph.dataModifiers.at(&dataOwner) ) {
      // create auxiliary variables indicating modifiers preceding the vertex
      index = index + model.addVariable(CP::Variable::Type::BOOLEAN, "precedes_{" + modifierExit.reference() + " → " + vertex->reference() + "}", position.at(&modifierExit) <= position.at(vertex) );
    }  
    // data index for data owner represents the number of modifiers exited according to the sequence positions
    dataIndices.emplace_back( model.addVariable(CP::Variable::Type::INTEGER, "data_index[" + dataOwner.node->id + "]_{" + vertex->reference() + "}", index ) );
  }
  dataIndex.emplace( vertex, std::move(dataIndices) );
}


const FlattenedGraph::Vertex* CPController::entry(const Vertex* vertex) {
/*
  assert( vertex->type == Vertex::Type::EXIT );
  return vertex - 1;
*/
  assert( flattenedGraph.vertexMap.contains({vertex->instanceId,vertex->loopIndices,vertex->node}));
  return &flattenedGraph.vertexMap.at({vertex->instanceId,vertex->loopIndices,vertex->node}).first;
}

const FlattenedGraph::Vertex* CPController::exit(const Vertex* vertex) {
/*
  assert( vertex->type == Vertex::Type::ENTRY );
  return vertex + 1;
*/
  assert( flattenedGraph.vertexMap.contains({vertex->instanceId,vertex->loopIndices,vertex->node}));
  return &flattenedGraph.vertexMap.at({vertex->instanceId,vertex->loopIndices,vertex->node}).second;
}

/*
bool CPController::checkEntryDependencies(NodeReference reference) {
  auto& [instance,node] = reference;
  if ( auto startEvent = node->represents<BPMN::UntypedStartEvent>() ) {
    if ( !entryStatus.contains({instance,startEvent->parent}) ) {
      return false;
    }
  }
  else if ( auto startEvent = node->represents<BPMN::TypedStartEvent>() ) {
    if ( !entryStatus.contains({instance,startEvent->parent->as<BPMN::EventSubProcess>()->parent}) ) {
      return false;
    }
    // TODO: check trigger dependencies?
    if ( startEvent->isInterrupting ) {
      throw std::runtime_error("CPController: unsupported interrupting start event '" + node->id + "'");
    }
    
    if ( auto messageStartEvent = node->represents<BPMN::MessageStartEvent>() ) {
      auto originCandidates = messageStartEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
      for ( auto candidate : originCandidates ) {
        for ( auto originInstance : originInstances[candidate->as<BPMN::MessageThrowEvent>()] ) {
          if ( !nodeVariables.contains({(size_t)originInstance,candidate}) ) {
            return false;
          }
        }
      }      
    }
    else {
      throw std::runtime_error("CPController: unsupported start event type for '" + node->id + "'");
    }
  }
  else if ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
    for ( const BPMN::SequenceFlow* sequenceFlow : flowNode->incoming ) {
      if ( !sequenceFlowVariables.contains({instance,sequenceFlow}) ) {
        return false;
      }
    }
  }
  else {
    assert(!"Unexpected node reference");
  }
  return true;
}

bool CPController::checkExitDependencies(NodeReference reference) {
  auto& [instance,node] = reference;
  if ( node->represents<BPMN::SendTask>() ) {
    // check all possible recipients
  }
  else if ( node->represents<BPMN::MessageCatchEvent>() ) {
    // check all possible senders
  }
  else if ( auto scope = node->represents<BPMN::Scope>() ) {
    // check that all exit variables of end flow nodes have been created
    for ( auto endNode : scope->flowNodes | std::ranges::views::filter([](const BPMN::FlowNode* node) { return node->outgoing.empty(); }) ) {
      if ( !exitStatus.contains({instance,endNode}) ) {
        return false;
      }
    }
  }

  return true;
}


void CPController::addEntryVariables(NodeReference reference) {
  auto& [instance,node] = reference;

  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  size_t newDataIndex = extensionElements->attributeRegistry.dataAttributes.size() - extensionElements->data.size();
  createNodeTraversalVariables(reference);
  
  if ( auto scope = node->represents<BPMN::Scope>() ) {
    // it is assumed that modified data states are only propagated upon entry and exit

    // determine sequential activities of sequential performers
    sequentialActivities[scope] = extensionElements->hasSequentialPerformer ? std::vector<const BPMN::Node* >() : scope->find_all(
      [scope](const BPMN::Node* candidate) {
        if ( auto activity = candidate->represents<BPMN::Activity>() ) {
          if ( auto adhocSubprocess = activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
            return adhocSubprocess->performer == scope;
          } 
        }
        return false;
      } 
    );

    for ( auto& attribute : extensionElements->data ) {
      // store scope in which data attributes live
      dataOwner[attribute.get()] = scope;
    }
  }

  // determine set holding the scopes for which entry data state variables are required
  for ( auto& [_,attribute] : extensionElements->attributeRegistry.dataAttributes ) {
    if ( auto flowNode = node->represents<BPMN::FlowNode>();
      flowNode && flowNode->parent != dataOwner.at(attribute)
    ) {
      auto scopeReference = ScopeReference{instance,dataOwner.at(attribute)};
      if ( flowNode->incoming.empty() ) {
        if ( auto activity = flowNode->parent->represents<BPMN::Activity>() ) {
          if ( auto adhoc = activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
           adhoc && adhoc->performer == dataOwner.at(attribute)
          ) {
            // flow node is start event of sequentially executed activity and data cannot be changed by any other activity
            continue;
          }
        }
        else {
          if (!entryDataState[NodeReference{instance,flowNode->parent}].contains(scopeReference) ) {
            // parent has no entry data state variable for attribute
            continue;
          }
        }
      }
      else {
        auto& predecessor = flowNode->incoming.front()->source;
        if (!entryDataState[NodeReference{instance,predecessor}].contains(scopeReference) ) {
          // predecessor has no entry data state variable for attribute
          continue;
        }
      }
    }
    scopesOwningData[reference].insert(dataOwner.at(attribute));
  }

  createEntryDataStateVariables(reference);
 
  // create entry data and new data state variables
  auto dataVariables = std::vector<AttributeVariables>();
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.dataAttributes ) {
    assert( attribute->index == dataVariables.size() );
    if ( attribute->index >= newDataIndex ) {
      // create new entry data variable
      auto value = scenario->getKnownValue(instance, attribute, 0); // get value known at time zero
      if ( value.has_value() ) {
        dataVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + name + "}", (double)true,(double)true ), 
          model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + name + "}", (double)value.value(), (double)value.value()) 
        );
      }
      else {
        dataVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + name + "}", (double)false,(double)false ), 
          model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + name + "}", 0.0, 0.0) 
        );
      }
      // create initial data state variables
      dataState[DataReference{instance,attribute}].emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN,std::string("defined^") + std::to_string(0) + "_{" + identifier(reference) + "}", dataVariables.back().defined ),
        model.addVariable(CP::Variable::Type::BOOLEAN,std::string("value^") + std::to_string(0) + "_{" + identifier(reference) + "}", dataVariables.back().value )
      );
      // create subsequent data state variables
      for ( size_t i = 1; i <= sequentialActivities[node->as<BPMN::Scope>()].size(); i++ ) {
        dataState[DataReference{instance,attribute}].emplace_back(
          model.addBinaryVariable(std::string("defined^") + std::to_string(i) + "_{" + identifier(reference) + "}"),
          model.addBinaryVariable(std::string("value^") + std::to_string(i) + "_{" + identifier(reference) + "}")
        );
      }
    }
    else {
      // deduce exisiting entry data variables
      auto scopeReference = ScopeReference{instance,dataOwner.at(attribute)};
      auto it = entryDataState[reference].find(scopeReference);
      if ( it == entryDataState.at(reference).end() ) {
        // deduce from parent or predecessor
        if ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
          deduceEntryAttributeVariable(dataVariables, attribute, reference, entryData, exitData);
        }
      }
      else {
        auto& entryDataStateVariables = it->second;
        CP::Cases defined_cases;
        CP::Cases value_cases;
        for ( size_t i = 0; i <= entryDataStateVariables.size(); i++ ) {
          auto& [defined,value] = dataState[DataReference{instance,attribute}][i];
          defined_cases.emplace_back( entryDataStateVariables[i], defined );
          value_cases.emplace_back( entryDataStateVariables[i], value );
        }
        dataVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_[" + identifier(reference) + "," + name+ "}", CP::n_ary_if(defined_cases,0) ), 
          model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + name+ "}", CP::n_ary_if(value_cases,0)) 
        );
      }
    }
  }
  entryData.emplace( reference , std::move(dataVariables) );

  // deduce exisiting entry status variables
  auto statusVariables = std::vector<AttributeVariables>();
  size_t newStatusIndex = extensionElements->attributeRegistry.statusAttributes.size() - extensionElements->attributes.size();
  for ( auto& [name,attribute] : node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry.statusAttributes ) {
    assert( attribute->index == statusVariables.size() );
    if ( attribute->index >= newStatusIndex ) {
      // create new entry status variable
      auto value = scenario->getKnownValue(instance, attribute, 0); // get value known at time zero
      if ( value.has_value() ) {
        statusVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + name + "}", (double)true,(double)true ), 
          model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + name + "}", (double)value.value(), (double)value.value()) 
        );
      }
      else {
        statusVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + name + "}", (double)false,(double)false ), 
          model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + name + "}", 0.0, 0.0) 
        );
      }
    }
    else {
      // deduce exisiting entry status variables
      if ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
        deduceEntryAttributeVariable(statusVariables, attribute, reference, entryStatus, exitStatus);
//// BEGIN COMMENT
        if ( flowNode->incoming.empty() ) {
          statusVariables.emplace_back(
            model.addVariable(CP::Variable::Type::BOOLEAN, "defined_entry_" + identifier(reference) + "_" + name, entryStatus.at(NodeReference{instance,flowNode->parent})[attribute->index].defined ), 
            model.addVariable(CP::Variable::Type::REAL, "value_entry_" + identifier(reference) + "_" + name, entryStatus.at(NodeReference{instance,flowNode->parent})[attribute->index].value ) 
          );
        }
        else if ( flowNode->incoming.size() == 1 ) {
          auto& predecessor = flowNode->incoming.front()->source;
          statusVariables.emplace_back(
            model.addVariable(CP::Variable::Type::BOOLEAN, "defined_entry_" + identifier(reference) + "_" + name, exitStatus.at(NodeReference{instance,predecessor})[attribute->index].defined ), 
            model.addVariable(CP::Variable::Type::REAL, "value_entry_" + identifier(reference) + "_" + name, exitStatus.at(NodeReference{instance,predecessor})[attribute->index].value ) 
          );
        }
        else if ( node->represents<BPMN::ExclusiveGateway>() ) {
          assert(!"Exclusive join not yet supported");
        }
        else {
          assert(!"Non-exclusive join not yet supported");
        }
//// END COMMENT
      }
    }
  }
  entryStatus.emplace( reference , std::move(statusVariables) );
}

void CPController::deduceEntryAttributeVariable(std::vector<AttributeVariables>& attributeVariables, const BPMNOS::Model::Attribute* attribute, NodeReference reference, variable_map<NodeReference,  std::vector<AttributeVariables> >& entryAttributes, variable_map<NodeReference, std::vector<AttributeVariables> >& exitAttributes) {
  auto& [instance,node] = reference;
  auto flowNode = node->as<BPMN::FlowNode>();
  
  if ( flowNode->incoming.empty() ) {
    // attribute values of start node is deduced from entry value of parent
    attributeVariables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + attribute->name + "}", entryAttributes.at(NodeReference{instance,flowNode->parent})[attribute->index].defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + attribute->name + "}", entryAttributes.at(NodeReference{instance,flowNode->parent})[attribute->index].value ) 
    );
  }
  else if ( flowNode->incoming.size() == 1 ) {
    // attribute values of flow node with exactle one predecessor is deduced from exit value of predecessor
    auto& predecessor = flowNode->incoming.front()->source;
    attributeVariables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined^entry_{" + identifier(reference) + "," + attribute->name, exitAttributes.at(NodeReference{instance,predecessor})[attribute->index].defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value^entry_{" + identifier(reference) + "," + attribute->name + "}", exitAttributes.at(NodeReference{instance,predecessor})[attribute->index].value ) 
    );
  }
  else if ( flowNode->represents<BPMN::ExclusiveGateway>() ) {
    assert(!"Exclusive join not yet supported");
  }
  else {
    assert(!"Non-exclusive join not yet supported");
  }
}


void CPController::createNodeTraversalVariables(NodeReference reference) {
  auto& [instance,node] = reference;
   
  // create node variables
  if ( node->represents<BPMN::Process>() ) {
    // process must be executed
    nodeVariables.emplace(
      NodeReference{instance, node},
      model.addVariable(CP::Variable::Type::BOOLEAN, std::string("x_{") + identifier(reference) + "}", (double)true, (double)true)
    );
  }
  else if ( auto startEvent = node->represents<BPMN::UntypedStartEvent>() ) {
    // start event must be executed if parent is executed
    nodeVariables.emplace(
      NodeReference{instance, node},
      model.addVariable(
        CP::Variable::Type::BOOLEAN, 
        std::string("x_{") + identifier(reference) + "}",
        nodeVariables.at(NodeReference{instance,startEvent->parent})
      )
    );
  }    
  else if ( auto startEvent = node->represents<BPMN::TypedStartEvent>() ) {
    if ( startEvent->isInterrupting ) {
      throw std::runtime_error("CPController: unsupported interrupting start event '" + node->id + "'");
    }
    
    if ( auto messageStartEvent = node->represents<BPMN::MessageStartEvent>() ) {
      CP::OrExpression messageReceived;
      auto originCandidates = messageStartEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
      for ( auto candidate : originCandidates ) {
        for ( auto originInstance : originInstances[candidate->as<BPMN::MessageThrowEvent>()] ) {
          messageReceived = messageReceived || messageFlowVariables.at(MessageCatchReference{instance,node->as<BPMN::MessageCatchEvent>()}).at(MessageThrowReference{(size_t)originInstance,candidate->as<BPMN::MessageThrowEvent>()});
        }
      }
      nodeVariables.emplace(
        NodeReference{instance, node},
        model.addVariable(
          CP::Variable::Type::BOOLEAN, 
          std::string("x_{") + identifier(reference) +"}",
          messageReceived
        )
      );
    }
    else {
      throw std::runtime_error("CPController: unsupported start event type for '" + node->id + "'");
    }
  }
  else if ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
    assert( flowNode->incoming.size() );
    CP::OrExpression tokenArrives;
    for (auto sequenceFlow : flowNode->incoming ) {
      tokenArrives = tokenArrives || sequenceFlowVariables.at(SequenceFlowReference{instance,sequenceFlow});
    }
    nodeVariables.emplace(
      NodeReference{instance, node},
      model.addVariable(
        CP::Variable::Type::BOOLEAN, 
        std::string("x_{") + identifier(reference) + "}",
        tokenArrives
      )
    );
  }
}


void CPController::createEntryDataStateVariables(NodeReference reference) {
 // create entry data states
  auto& [instance,node] = reference;
//  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

 
  // the entry data state is non-decreasing along sequence flows and subprocesses 
  for ( auto scopeOwningData : scopesOwningData.at(reference) ) {
    auto scopeReference = ScopeReference{instance,node->as<BPMN::Scope>()};
    std::vector< std::reference_wrapper< const CP::Variable > > variables;
    CP::LinearExpression sumVariables(0);
    for ( size_t i = 0; i <= sequentialActivities[scopeOwningData].size(); i++ ) {
      if ( scopeOwningData == node ) {
        // use first entry data state
        variables.emplace_back(
          model.addVariable( CP::Variable::Type::BOOLEAN, std::string("y^{entry,") + std::to_string(i) + "," + scopeOwningData->id + "}_{" + identifier(reference) + "}", (double)(i == 0), (double)(i == 0) ) 
        );
      }
      else {
        if ( auto activity = node->represents<BPMN::Activity>() ) {
          // for each activity, the entry data state is a decision (with constraints)
          if ( activity->incoming.size() > 1 ) {
            throw std::runtime_error("CPController: implicit join for activity '" + activity->id + "'");
          }
          else if ( activity->incoming.empty() && !activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
            throw std::runtime_error("CPController: activitiy '" + activity->id + "' has no incoming sequence flow");
          }
          
          if ( activity->incoming.size() == 1 ) {
            if ( !activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
              variables.emplace_back(
                model.addBinaryVariable( std::string("y^{entry,") + std::to_string(i) + "," + scopeOwningData->id +  "}_{" + identifier(reference) + "}" ) 
              );
            }
            // ensure that first i entry data states are smaller or equal first i exit data states of predecessor
            auto& predecessor = activity->incoming.front()->source;
            CP::LinearExpression sumFirstExitDataStatesOfPredecessor(0);
            CP::LinearExpression sumFirstEntryDataStatesOfActivity(0);
            for ( size_t j = 0; j <= i ; j++ ) {
              sumFirstExitDataStatesOfPredecessor += exitDataState.at(NodeReference{instance,predecessor}).at(scopeReference)[j].get();
              sumFirstEntryDataStatesOfActivity += variables[j].get();
            }
            model.addConstraint( sumFirstEntryDataStatesOfActivity <= sumFirstExitDataStatesOfPredecessor );
          }
          
        }
        else if ( auto startEvent = node->represents<BPMN::UntypedStartEvent>() ) {
          // for each untyped start event, the entry data state must be the same as for its parent
          auto& parentEntryDataState = entryDataState.at(NodeReference{instance,startEvent->parent}).at( scopeReference )[i].get();
          variables.emplace_back(
            model.addVariable( CP::Variable::Type::BOOLEAN, std::string("y^{entry,") + std::to_string(i) + "," + scopeOwningData->id + "}_{" + identifier(reference) + "}", parentEntryDataState ) 
          );
        }
        else if ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
          assert( flowNode->incoming.size() );
          assert(!node->represents<BPMN::Activity>());
          
          if ( flowNode->incoming.size() == 1 ) {
            // entry data states must be the same as exit state of predecessor
            auto& predecessor = flowNode->incoming.front()->source;
            auto& predecessorExitDataState = exitDataState.at(NodeReference{instance,predecessor}).at( scopeReference )[i].get();
            variables.emplace_back(
              model.addVariable( CP::Variable::Type::BOOLEAN, std::string("y^{entry,") + std::to_string(i) + "," + scopeOwningData->id + "}_{" + identifier(reference) + "}", predecessorExitDataState ) 
            );
          }
          else if ( node->represents<BPMN::ExclusiveGateway>() ) {
            assert(!"Exclusive join not yet supported");
          }
          else {
            assert(!"Non-exclusive join not yet supported");
          }
        }        
      }
      sumVariables += variables.back().get();
    }
    entryDataState[reference].emplace( scopeReference, std::move(variables) );

    // if and only if the node is visited, exactly one of the variables must be true
    model.addConstraint( sumVariables == nodeVariables.at(reference) );

  }

}

void CPController::addChildren(ScopeReference reference) {
  auto& [instance,scope] = reference;
  if ( scope->startNodes.empty() ) {
    // no flow nodes within scope
    return;
  }
  else if ( scope->startNodes.size() > 1 ) {
    throw std::runtime_error("CPController: multiple start nodes for scope '" + scope->id + "'");
  }
  pendingEntry.insert(NodeReference{instance,scope->startNodes.front()});

  for ( auto eventSubProcess : scope->eventSubProcesses ) {
    assert(!"Event-subprocesses not yet supported");
  }

}

void CPController::addSuccessors(NodeReference reference) {
  auto& [instance,node] = reference;
  assert(node->represents<BPMN::FlowNode>());
  auto flowNode = node->as<BPMN::FlowNode>();
  if ( flowNode->outgoing.size() == 1 || node->represents<BPMN::ParallelGateway>() ) {
    for ( auto sequenceFlow : flowNode->outgoing ) {
      auto& successor = sequenceFlow->target;
      // token traverses sequence flow when node is visited
      sequenceFlowVariables.emplace(
        SequenceFlowReference{instance, sequenceFlow},
        model.addVariable(
          CP::Variable::Type::BOOLEAN, 
          std::string("x^sequence_{") + identifier(reference) + "," + successor->id + "}",
          nodeVariables.at(reference)
        )
      );
      pendingEntry.insert(NodeReference{instance,successor});
    }
  }
  else if ( node->represents<BPMN::ExclusiveGateway>() || node->represents<BPMN::InclusiveGateway>() ) {
    assert(!"Exclusive and inclusive fork not yet supported");
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    assert(!"Event-based gateways not yet supported");
  }
}

void CPController::addExitVariables(NodeReference reference) {
  auto& [instance,node] = reference;
  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  
  createExitDataStateVariables(reference);

  // create exit data variables
  auto dataVariables = std::vector<AttributeVariables>();
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.dataAttributes ) {
    auto scopeOwningData = dataOwner.at(attribute);
    assert( attribute->index == dataVariables.size() );


    auto scopeReference = ScopeReference{instance,dataOwner.at(attribute)};
    auto it = exitDataState[reference].find(scopeReference);
    if ( it != exitDataState.at(reference).end() ) {
      auto& exitDataStateVariables = it->second;
      CP::Cases defined_cases;
      CP::Cases value_cases;
      for ( size_t i = 0; i < exitDataStateVariables.size(); i++ ) {
        auto& [defined,value] = dataState[DataReference{instance,attribute}][i];
        defined_cases.emplace_back( exitDataStateVariables[i], defined );
        value_cases.emplace_back( exitDataStateVariables[i], value );
      }
      dataVariables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined^exit_[" + identifier(reference) + "," + name+ "}", CP::n_ary_if(defined_cases,0) ), 
        model.addVariable(CP::Variable::Type::REAL, "value^exit_{" + identifier(reference) + "," + name+ "}", CP::n_ary_if(value_cases,0)) 
      );
    }
  }
  exitData.emplace( reference , std::move(dataVariables) );

  // create exit status variables
  // TODO
  auto statusVariables = std::vector<AttributeVariables>();
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.statusAttributes ) {
    assert( attribute->index == statusVariables.size() );
    if ( auto scope = node->represents<BPMN::Scope>() ) {
      // create exit status variables
      if ( scope->flowNodes.empty() ) {
        // exit status is the same as entry status
        statusVariables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined^exit_[" + identifier(reference) + "," + name+ "}", entryStatus.at(reference)[attribute->index].defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value^exit_{" + identifier(reference) + "," + name+ "}", entryStatus.at(reference)[attribute->index].value ) 
        );
      }
      else {
        // assume an exclusive end node
      }
    }
    else if ( auto event = node->represents<BPMN::CatchEvent>() ) {
    }
    else if ( auto gateway = node->represents<BPMN::Gateway>();
      gateway && gateway->incoming.size() > 1
    ) {
    }
    else if ( auto task = node->represents<BPMN::Task>() ) {
    }
    else {
    }
  }
}


void CPController::createExitDataStateVariables(NodeReference reference) {
  auto& [instance,node] = reference;
  for ( auto scopeOwningData : scopesOwningData.at(reference) ) {
    std::vector< std::reference_wrapper< const CP::Variable > > variables;
    CP::LinearExpression sumVariables(0);
    for ( size_t i = 0; i <= sequentialActivities[scopeOwningData].size(); i++ ) {
      // TODO
      if ( auto activity = node->represents<BPMN::Activity>() ) {
        if ( auto adhoc = activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
          adhoc && adhoc->performer == scopeOwningData 
        ) {
          // TODO: exit data state must be exactly one higher than entry data state of each child
        }
        else {
          // for all other activities, the exit data state is a decision (with constraints)
          variables.emplace_back(
            model.addBinaryVariable( std::string("y^{exit_{") + std::to_string(i) + "," + scopeOwningData->id + "}_{" + identifier(reference) + "}" ) 
          );

          if ( auto scope = node->represents<BPMN::Scope>() ) {
            constrainExitDataStateVariables(ScopeReference{instance,scope});
          }
        }
      }
      else if ( auto flowNode = node->represents<BPMN::CatchEvent>() ) {
        // TODO: exit data state must be higher than exit data state of each child
      }
      else if ( auto process = node->represents<BPMN::Process>() ) {
          // the exit data state is a decision (with constraints)
          variables.emplace_back(
            model.addBinaryVariable( std::string("y^{exit,") + std::to_string(i) + "," + scopeOwningData->id + "}_{" + identifier(reference) + "}" ) 
          );
          constrainExitDataStateVariables(ScopeReference{instance,process});
      }
      else {
        // TODO: exit data state must be the same as entry data state
      }
      sumVariables += variables.back().get();

      // ensure that first i exit data states are smaller or equal first i entry data states
      CP::LinearExpression sumFirstEntryDataStates(0);
      CP::LinearExpression sumFirstExitDataStates(0);
      for ( size_t j = 0; j <= i ; j++ ) {
        sumFirstEntryDataStates += entryDataState.at(reference).at(ScopeReference{instance,scopeOwningData})[j].get();
        sumFirstExitDataStates += variables[j].get();
      }
      model.addConstraint( sumFirstExitDataStates <= sumFirstEntryDataStates );
    }
    exitDataState[reference].emplace( ScopeReference{instance,scopeOwningData}, std::move(variables) );
    // if and only if the node is visited, exactly one of the variables must be true
    model.addConstraint( sumVariables == nodeVariables.at(reference) );
  }
}

void CPController::constrainExitDataStateVariables(ScopeReference reference) {
  // TODO: exit data state of scope must be higher or equal than exit data state of each descendant
}

void CPController::createSequenceFlowTraversalVariables(SequenceFlowReference reference) {
}
*/

///  TODO: ensure that the highest entry data state remains smaller than the sum of sequential activities beeing used 
///  TODO: ensure that the highest exit data state remains smaller or equal than the sum of sequential activities beeing used 
///  TODO: add constraint that the the sum of entry data state variables for each sequential activity and each data state is one

