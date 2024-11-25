#include "CPController.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"

#include <iostream>

using namespace BPMNOS::Execution;

CPController::CPController(const BPMNOS::Model::Scenario* scenario)
 : scenario(scenario)
 , flattenedGraph(FlattenedGraph(scenario))
 , model(CP::Model::ObjectiveSense::MAXIMIZE)
{
  createCP();
}

void CPController::connect(Mediator* mediator) {
  Controller::connect(mediator);
}

std::shared_ptr<Event> CPController::dispatchEvent(const SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
  // TODO
  return best;
}

void CPController::createCP() {
  vertices.reserve( flattenedGraph.vertices.size() );
std::cerr << "initializeVertices" << std::endl;
  // determine relevant vertices of all process instances
  for ( auto initialVertex : flattenedGraph.initialVertices ) {
    initializeVertices(initialVertex);
  }

std::cerr << "create sequence position variables" << std::endl;
  // create sequence position variables for all vertices
  auto sequence = model.addSequence( "sequence", vertices.size() );
  for ( size_t i = 0; i < vertices.size(); i++ ) {
    position.emplace(vertices[i], sequence[i]);
  } 

std::cerr << "createGlobalVariables" << std::endl;
  createGlobalVariables();

std::cerr << model.stringify() << std::endl;  
std::cerr << "createVertexVariables" << std::endl;
  // create vertex and message variables
  for ( auto vertex : vertices ) {
    createVertexVariables(*vertex);
  }
  
std::cerr << "createMessageVariables" << std::endl;
  createMessageVariables();
std::cerr << "Done" << std::endl;
}

void CPController::createGlobalVariables() {
  for ( auto& [name,attribute] : scenario->model->attributeRegistry.globalAttributes ) {
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
      defined.emplace_back((double)true,(double)true);
      value.emplace_back((double)initialValue.value(), (double)initialValue.value()); 
    }
    else {
      // undefined initial value
      defined.emplace_back((double)false,(double)false);
      value.emplace_back(0.0, 0.0); 
    }
    
    if ( attribute->isImmutable ) {
      // deduced variables
      for ( [[maybe_unused]] auto _ : flattenedGraph.globalModifiers ) {
        defined.emplace_back(defined[0]);
        value.emplace_back(value[0]); 
      }
    }
    else {
      for ( [[maybe_unused]] auto _ : flattenedGraph.globalModifiers ) {
        // unconstrained variables
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
      messages.emplace_back( model.addBinaryVariable("message_" + sender.reference() + "_" + recipient->reference() ) );
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
      if ( sender.node->represents<BPMN::SendTask>() ) {
        auto senderExit = &sender + 1;
        model.addConstraint(
          message.implies (
            status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
            <= status.at(senderExit)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          )
        );
      }

      // TODO: add message header constraints
    }
    CP::LinearExpression sum;
    for ( const CP::Variable& message : messages ) {
      sum += message;
    }
    model.addConstraint( sum == visit.at(recipient) );
  }
}

void CPController::createMessageContent(const Vertex& vertex) {
  // TODO
  // every visited recipient must receive a message
  // message precedence constraints
  
  // message content
}

void CPController::createDataVariables(const FlattenedGraph::Vertex& vertex) {
  auto extensionElements = vertex.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  std::vector< IndexedAttributeVariables > variables;
  
  for ( auto& attribute : extensionElements->data ) {
    assert( attribute->index == variables.size() ); // ensure that the order of attributes is correct
    variables.emplace_back(
      model.addIndexedVariables(CP::Variable::Type::BOOLEAN, "defined_" + BPMNOS::to_string(vertex.instanceId,STRING) + "," + attribute->id ), 
      model.addIndexedVariables(CP::Variable::Type::REAL, "value_" + BPMNOS::to_string(vertex.instanceId,STRING) + "," + attribute->id) 
    );
    auto& [defined,value] = variables.back();
    // add variables holding initial values
    auto initialValue = scenario->getKnownValue(vertex.rootId, attribute.get(), 0);
    if ( initialValue.has_value() ) {
      // defined initial value
      defined.emplace_back((double)true,(double)true);
      value.emplace_back((double)initialValue.value(), (double)initialValue.value()); 
    }
    else {
      // undefined initial value
      defined.emplace_back((double)false,(double)false);
      value.emplace_back(0.0, 0.0); 
    }
    
    if ( attribute->isImmutable ) {
      // deducible variables
      for ( [[maybe_unused]] auto _ : flattenedGraph.dataModifiers.at(&vertex) ) {
        defined.emplace_back(defined[0]);
        value.emplace_back(value[0]); 
      }
    }
    else {
      for ( [[maybe_unused]] auto _ : flattenedGraph.dataModifiers.at(&vertex) ) {
        // unconstrained variables
        defined.emplace_back();
        value.emplace_back(); 
      }
    }
  }

  data.emplace( &vertex, std::move(variables) ); 
}

void CPController::createStatus(const Vertex& vertex) {
  if ( vertex.entry<BPMN::UntypedStartEvent>() ) {
    // TODO: start-event
    assert( vertex.predecessors.size() == 1 );
    createAlternativeEntryStatus(vertex, {{visit.at(&vertex.predecessors.front().get()), status.at(&vertex.predecessors.front().get())}} );
  }
  else if ( vertex.entry<BPMN::TypedStartEvent>() ) {
    // TODO: start-event
  }
  else if ( vertex.entry<BPMN::ExclusiveGateway>() && vertex.inflows.size() > 1 ) {
    std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives;
    for ( auto& [sequenceFlow,predecessor] : vertex.inflows ) {
      // add to alternatives
      alternatives.emplace_back( tokenFlow.at({&predecessor,&vertex}), statusFlow.at({&predecessor,&vertex}) );
    }
    createAlternativeEntryStatus(vertex, std::move(alternatives));
  }
  else if ( vertex.entry<BPMN::FlowNode>() && vertex.inflows.size() > 1 ) {
    assert(vertex.entry<BPMN::ParallelGateway>() || vertex.entry<BPMN::InclusiveGateway>() );
    assert( vertex.predecessors.size() == 1 );
    if ( !vertex.entry<BPMN::ParallelGateway>() && !vertex.entry<BPMN::InclusiveGateway>() ) {
      throw std::runtime_error("CPController: illegal join at '" + vertex.node->id + "'");
    }
    std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs;
    for ( auto& [sequenceFlow,predecessor] : vertex.inflows ) {
      // add to alternatives
      inputs.emplace_back( tokenFlow.at({&predecessor,&vertex}), statusFlow.at({&predecessor,&vertex}) );
    }
    createMergedStatus(vertex, std::move(inputs));
  }
  else if ( vertex.entry<BPMN::FlowNode>() ) {
    assert( vertex.inflows.size() == 1 );
    auto& [sequenceFlow,predecessor] = vertex.inflows.front();
    createAlternativeEntryStatus(vertex, {{tokenFlow.at({&predecessor,&vertex}), statusFlow.at({&predecessor,&vertex})}} );
  }

  // TODO: sequential activity or multi-instance sequential activity
}

void CPController::createAlternativeEntryStatus(const Vertex& vertex, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives) {
  assert( vertex.type == Vertex::Type::ENTRY );
  auto extensionElements = vertex.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  std::vector<AttributeVariables> variables;
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.statusAttributes ) {
    if ( attribute->index < extensionElements->attributeRegistry.statusAttributes.size() - extensionElements->attributes.size() ) {
      // deduce variable
      CP::BooleanExpression defined(false);
      CP::LinearExpression value = 0.0;
      for ( auto& [ active, attributeVariables] : alternatives ) {
        defined = defined || attributeVariables[attribute->index].defined;
        value = value + attributeVariables[attribute->index].value;
      }
    }
    else {
      // add new attributes
      assert( vertex.type == Vertex::Type::ENTRY );
      // add variables holding given values
      auto value = scenario->getKnownValue(vertex.rootId, attribute, 0);
    
      if ( value.has_value() ) {
        // defined initial value
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, (double)true,(double)true ), 
          model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, (double)value.value(), (double)value.value()) 
        );
      }
      else {
        // no given value
        bool defined = ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp );
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, (double)defined,(double)defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, 0.0, 0.0) 
        );
      }
    }
  }

  status.emplace( &vertex, std::move(variables) );
}

void CPController::createMergedStatus(const Vertex& vertex, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs) {
  assert( ( vertex.type == Vertex::Type::ENTRY && vertex.inflows.size() > 1) || vertex.exit<BPMN::Scope>() );
  auto extensionElements = vertex.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  std::vector<AttributeVariables> variables;
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.statusAttributes ) {
    if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      CP::MaxExpression timestamp;
      for ( auto& [ active, attributeVariables] : inputs ) {
        timestamp = CP::max( timestamp, attributeVariables[attribute->index].value );
      }
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, (double)true, (double)true ), 
        model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, timestamp )
      );
    }
    else if ( attribute->index < extensionElements->attributeRegistry.statusAttributes.size() - extensionElements->attributes.size() ) {
      // deduce variable
      CP::BooleanExpression defined(false);
      CP::Cases cases;
      for ( auto& [ active, attributeVariables] : inputs ) {
        defined = defined || attributeVariables[attribute->index].defined;
        cases.emplace_back( attributeVariables[attribute->index].defined, attributeVariables[attribute->index].value );
      }
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, defined ), 
        model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, CP::n_ary_if( cases, 0.0 ))
      );      
      // add constraints that all defined inputs have the same value
      auto& mergedValue = variables.back().value;
      for ( auto& [ _, attributeVariables] : inputs ) {
        auto& [ defined, value ] = attributeVariables[attribute->index];
        model.addConstraint( defined.implies( value == mergedValue ) );
      }
    }
    else {
      // add new attributes
      assert( vertex.type == Vertex::Type::ENTRY );
      // add variables holding given values
      auto value = scenario->getKnownValue(vertex.rootId, attribute, 0);
    
      if ( value.has_value() ) {
        // defined initial value
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, (double)true,(double)true ), 
          model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, (double)value.value(), (double)value.value()) 
        );
      }
      else {
        // no given value
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, (double)false,(double)false ), 
          model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, 0.0, 0.0) 
        );
      }
    }
  }

  status.emplace( &vertex, std::move(variables) );
}

void CPController::createExitStatus(const Vertex& vertex) {
  assert( vertex.type == Vertex::Type::EXIT );
  assert( !vertex.exit<BPMN::Scope>() );
  const Vertex& entry = *(&vertex - 1);
  auto extensionElements = vertex.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  // From Token.cpp:
  // - process operators are applied upon entry
  // - subprocess operators are applied upon entry
  // - operators of receive task and decision task are applied on completion
  // - event subprocess operators are applied when exiting typed start event  

  // copy entry status references
  std::vector<AttributeVariables> currentStatus = status.at(&entry);
  //std::vector<AttributeVariables> currentData = status.at(&entry);
  //std::vector<AttributeVariables> currentGlobla = status.at(&entry);
  
  if ( vertex.node->represents<BPMNOS::Model::DecisionTask>() ) {
    // apply choices
    // TODO
  }

  if ( vertex.node->represents<BPMN::MessageCatchEvent>() ) {
    // receive message content (globals and data attributes must not be changed)
    if ( extensionElements->messageDefinitions.size() == 1 ) {
      std::vector< std::tuple< std::string_view, size_t, AttributeVariables> > contentVariables;
      auto& messageDefinition = extensionElements->messageDefinitions.front();
      for (auto& [key,content] : messageDefinition->contentMap ) {
        if ( !content->attribute.has_value() ) {
          throw std::runtime_error("CPController: no attribute specified for receiving message content '" + content->id + "'");
        }
        auto attribute = &content->attribute.value().get();
        
        // create variables for the message content (relevant constraints must be added separately)
        AttributeVariables attributeVariables = {
          model.addBinaryVariable("content[" + content->key + "]_defined_" + vertex.reference() + "," + attribute->id ), 
          model.addRealVariable("content[" + content->key + "]_value_" + vertex.reference() + "," + attribute->id )
        };

        // use message content variables in current status
        currentStatus.at(attribute->index) = attributeVariables;

        contentVariables.emplace_back( content->key, attribute->index, std::move(attributeVariables) );
      }

      messageContent.emplace( &vertex, std::move(contentVariables) );
    }
    else if ( extensionElements->messageDefinitions.size() > 1 ) {
      // multi-instance receive task
      assert(!"Not yet supported");
    }
  }
  
  if ( vertex.node->represents<BPMN::MessageStartEvent>() ) {
    // add entry restrictions of event-subprocess
    // TODO
  }

  if ( vertex.node->represents<BPMN::Task>() ) {
    // apply operators
    // TODO
  }

  std::vector<AttributeVariables> variables;
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  for ( auto& [name,attribute] : extensionElements->attributeRegistry.statusAttributes ) {
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + vertex.reference() + "," + attribute->id, currentStatus[attribute->index].defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_" + vertex.reference() + "," + attribute->id, currentStatus[attribute->index].value )
    );      

    // ensure that attribute only has a value if vertex is visited and the attribute if defined 
    model.addConstraint( status.at(&vertex)[attribute->index].defined <= visit.at(&vertex) );
    model.addConstraint( (!status.at(&vertex)[attribute->index].defined).implies( status.at(&vertex)[attribute->index].value == 0.0 ) );
  }
  status.emplace( &vertex, std::move(variables) );
}

std::vector< std::reference_wrapper<const FlattenedGraph::Vertex> > CPController::getSortedVertices(const FlattenedGraph::Vertex& initialVertex) {
  std::vector< std::reference_wrapper<const FlattenedGraph::Vertex> > sortedVertices;
  std::unordered_map<const FlattenedGraph::Vertex*, size_t> inDegree;
  
  std::deque<const FlattenedGraph::Vertex*> queue;
  queue.push_back(&initialVertex);
    
  // determine vertices in topological order
  while ( !queue.empty() ) {
    const FlattenedGraph::Vertex* current = queue.front();
    queue.pop_front();
    sortedVertices.push_back(*current);
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
      // decrement in degree and add vertex to queue if zero
      if ( --inDegree.at(&vertex) == 0 ) {
        queue.push_back(&vertex);
      }
    }
  }
  
  if ( inDegree.size() ) {
    throw std::runtime_error("CPController: cycle detected in '" + BPMNOS::to_string(initialVertex.rootId, STRING) + "'");
  }

  return sortedVertices;
}

void CPController::initializeVertices(const FlattenedGraph::Vertex& initialVertex) {
  for ( const Vertex& vertex : getSortedVertices(initialVertex) ) {
    vertices.push_back(&vertex);
  }
}

void CPController::createVertexVariables(const FlattenedGraph::Vertex& vertex) {
std::cerr << "createGlobalIndexVariable" << std::endl;
  createGlobalIndexVariable(vertex);
std::cerr << "createDataIndexVariables" << std::endl;
  createDataIndexVariables(vertex);
  
  if ( vertex.type == Vertex::Type::ENTRY ) {
std::cerr << "createEntryVariables" << std::endl;
    createEntryVariables(vertex);
  }
  else {
std::cerr << "createExitVariables" << std::endl;
    createExitVariables(vertex);
  }

std::cerr << "createStatus" << std::endl;
  createStatus(vertex);
  
std::cerr << "createSequenceConstraints" << std::endl;
  createSequenceConstraints(vertex);
}

void CPController::createEntryVariables(const FlattenedGraph::Vertex& vertex) {
  // visit variable
  if ( vertex.node->represents<BPMN::UntypedStartEvent>() ) {
    assert( vertex.inflows.empty() );
    assert( vertex.predecessors.size() == 1 );
    assert( vertex.senders.empty() );
    
    // deduce visit from parent
    auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_" + BPMNOS::to_string(vertex.instanceId, STRING) + "," + vertex.node->id, visit.at( &vertex.parent().first ) );
    visit.emplace(&vertex, deducedVisit );
    visit.emplace(&vertex+1, deducedVisit );
  }
  else if ( vertex.node->represents<BPMN::TypedStartEvent>() ) {
    assert(!"Not yet implemented");
  }
  else if ( vertex.node->represents<BPMN::FlowNode>() ) {
    if ( vertex.inflows.size() == 1 ) {
      // deduce visit from unique sequence flow
      auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_" + BPMNOS::to_string(vertex.instanceId, STRING) + "," + vertex.node->id , tokenFlow.at( std::make_pair(&vertex.inflows.front().second, &vertex) ) );
      visit.emplace(&vertex, deducedVisit );
      visit.emplace(&vertex+1, deducedVisit );
    }
    else {
      assert(!"Not yet implemented");
    }
  }
  else if ( vertex.node->represents<BPMN::Process>() ) {
    // every process vertex is visited
    auto& knownVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_" + BPMNOS::to_string(vertex.instanceId, STRING) + "," + vertex.node->id, (double)true, (double)true );
    visit.emplace(&vertex, knownVisit );
    visit.emplace(&vertex+1, knownVisit );
  }

  // status attributes
  // message content
}

void CPController::createExitVariables(const FlattenedGraph::Vertex& vertex) {
  // status attributes
  // message content

  if ( vertex.exit<BPMN::FlowNode>() ) {
    // flow variables
    if ( vertex.outflows.size() == 1 ) {
      tokenFlow.emplace( 
        std::make_pair(&vertex,&vertex.outflows.front().second), 
        model.addVariable(CP::Variable::Type::BOOLEAN, "tokenflow_" + vertex.reference() + "_" + vertex.outflows.front().second.reference(), visit.at(&vertex) ) 
      );
      auto extensionElements = vertex.parent().first.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      std::vector<AttributeVariables> variables;
      variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
      for ( auto& [name,attribute] : extensionElements->attributeRegistry.statusAttributes ) {
        // deduce variable
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "statusflow_defined_" + vertex.reference() + "_" + vertex.outflows.front().second.reference() + "," + attribute->id, 
            CP::if_then_else( 
              tokenFlow.at({&vertex,&vertex.outflows.front().second}), 
              status.at(&vertex)[attribute->index].defined, 
              (double)false
            )
          ), 
          model.addVariable(CP::Variable::Type::REAL, "statusflow_value_" + vertex.reference() + "_" + vertex.outflows.front().second.reference() + "," + attribute->id, 
            CP::if_then_else( 
              tokenFlow.at({&vertex,&vertex.outflows.front().second}), 
              status.at(&vertex)[attribute->index].value, 
              0.0
            )
          )
        );
      }      

      statusFlow.emplace( 
        std::make_pair(&vertex,&vertex.outflows.front().second), 
        std::move(variables)
      );
    }
    else {
      assert(!"Not yet implemented");
    }
  }
  else {
    assert(!"Not yet implemented");
  }

}

void CPController::createSequenceConstraints(const Vertex& vertex) {
  auto addConstraints = [&](const Vertex& predecessor, const Vertex& vertex) {
    model.addConstraint( position.at(&predecessor) <= position.at(&vertex) );
    model.addConstraint(
      // if a vertex is visited, its timestamp must not be before the predecessors timestamp
      // as all timestamps are non-negative and zero if not visited, no condition on a visit
      // of the predecessor is required
      visit.at(&vertex).implies (
        status.at(&predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        <= status.at(&predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
      )
    );
  };

  for ( auto& [sequenceFlow, predecessor] : vertex.inflows ) {
    addConstraints(predecessor,vertex);
  }

  for ( auto& predecessor : vertex.predecessors ) {
    addConstraints(predecessor,vertex);
  }
}


void CPController::createGlobalIndexVariable(const Vertex& vertex) {
  CP::LinearExpression index;
  for ( auto& [modifierEntry, modifierExit] : flattenedGraph.globalModifiers ) {
    // create auxiliary variables indicating modifiers preceding the vertex
    index += model.addVariable(CP::Variable::Type::BOOLEAN, "precedes_" + modifierExit.reference() + "_" + vertex.reference(), position.at(&modifierExit) <= position.at(&vertex) );
  }  
  globalIndex.emplace( &vertex, model.addVariable(CP::Variable::Type::INTEGER, "globals_index_" + vertex.reference(), index ) );
}


void CPController::createDataIndexVariables(const Vertex& vertex) {
  std::vector<  CP::reference_vector< const CP::Variable > > dataIndices;
  dataIndices.resize( vertex.dataOwners.size() );
  for ( size_t i = 0; i < vertex.dataOwners.size(); i++ ) {
    auto& dataOwner = vertex.dataOwners[i].get();
    assert( flattenedGraph.dataModifiers.contains(&dataOwner) ); 
  
    CP::LinearExpression index;
    for ( auto& [modifierEntry, modifierExit] : flattenedGraph.dataModifiers.at(&dataOwner) ) {
      // create auxiliary variables indicating modifiers preceding the vertex
      index += model.addVariable(CP::Variable::Type::BOOLEAN, "precedes_" + modifierExit.reference() + "_" + vertex.reference(), position.at(&modifierExit) <= position.at(&vertex) );
    }  
    dataIndices[i].emplace_back( model.addVariable(CP::Variable::Type::INTEGER, "data_index_" + vertex.reference() + "," + std::to_string(i) , index ) );
  }
  dataIndex.emplace( &vertex, std::move(dataIndices) );
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

