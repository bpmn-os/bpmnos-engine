#include "CPModel.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include <limex_handle.h>
#include <iostream>

using namespace BPMNOS::Execution;

CPModel::CPModel(const BPMNOS::Execution::FlattenedGraph& flattenedGraph, Config config)
 : scenario(flattenedGraph.scenario)
 , config(std::move(config))
 , flattenedGraph(flattenedGraph) 
 , model(CP::Model::ObjectiveSense::MAXIMIZE)
{
//std::cerr << "Flattened graph: " << flattenedGraph.jsonify().dump() << std::endl;
  // add callables for lookup tables
  for ( auto& lookupTable : scenario.model->lookupTables ) {
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

//std::cerr << "Limex handles: ";
//for ( auto name : limexHandle.getNames() ) {
//  std::cerr << name << ", ";
//}
//std::cerr << std::endl;

  createCP();
}

const BPMNOS::Model::Scenario& CPModel::getScenario() const {
  return scenario;
}
std::optional< BPMN::Activity::LoopCharacteristics> CPModel::getLoopCharacteristics(const Vertex* vertex) const {
  auto activity = vertex->node->represents<BPMN::Activity>();
  if ( !activity ) {
    return std::nullopt;
  }
  return activity->loopCharacteristics;
}


void CPModel::createCP() {
  auto sortedVertices = flattenedGraph.getSortedVertices();

//std::cerr << "create sequence position variables" << std::endl;
  // create sequence position variables for all vertices
  auto& sequence = model.addSequence( "position", sortedVertices.size() );
  for ( size_t i = 0; i < sortedVertices.size(); i++ ) {
    position.emplace(sortedVertices[i], sequence.variables[i]);
  } 

//std::cerr << "createMessageFlowVariables" << std::endl;
  createMessageFlowVariables();

//std::cerr << "createGlobalVariables" << std::endl;
  createGlobalVariables();

//std::cerr << "createVertexVariables:" << flattenedGraph.vertices.size() << std::endl;
  // create vertex and message variables
  for ( auto vertex : sortedVertices ) {
    createVertexVariables(vertex);
  }

//std::cerr << "constrainGlobalVariables" << std::endl;
  constrainGlobalVariables();

  for ( auto vertex : sortedVertices ) {
    if ( vertex->entry<BPMN::Scope>() ) {
//std::cerr << "constrainDataVariables " << vertex->reference() << std::endl;
      constrainDataVariables(vertex);
    }
    if ( vertex->exit<BPMN::EventBasedGateway>() ) {
//std::cerr << "constrainEventBasedGateway " << vertex->reference() << std::endl;
      constrainEventBasedGateway(vertex);
    }
    if ( vertex->exit<BPMN::TypedStartEvent>() ) {
      constrainTypedStartEvent(vertex);
    }
  }  

//std::cerr << "constrainSequentialActivities" << std::endl;
  constrainSequentialActivities();
  
//std::cerr << "createMessagingConstraints" << std::endl;
  createMessagingConstraints();
//std::cerr << "Done" << std::endl;
//std::cerr << model.stringify() << std::endl;  
}

void CPModel::createGlobalVariables() {
  for ( auto attribute : scenario.model->attributeRegistry.globalAttributes ) {
    assert( attribute->index == globals.size() ); // ensure that the order of attributes is correct
    globals.emplace_back(
      model.addIndexedVariables(CP::Variable::Type::BOOLEAN, "defined_" + attribute->id ), 
      model.addIndexedVariables(CP::Variable::Type::REAL, "value_" + attribute->id) 
    );
    auto& [defined,value] = globals.back();
    // add variables holding initial values
    auto& initialValue = scenario.globals[attribute->index];
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
    addToObjective( attribute, value[ value.size() -1 ] );
  }
}

void CPModel::createMessageFlowVariables() {
  for ( auto& vertex : flattenedGraph.vertices ) {
    if ( vertex.exit<BPMN::MessageCatchEvent>() ) {
      messageRecipients.push_back(&vertex);
      CP::reference_vector<const CP::Variable> messages;
      for ( Vertex& sender : vertex.senders ) {
        assert( sender.entry<BPMN::MessageThrowEvent>() );
        // create binary decision variable for a message from sender to recipient
        messages.emplace_back( model.addBinaryVariable("message_{" + sender.reference() + " → " + vertex.reference() + "}" ) );
//std::cerr << messages.back().get().stringify() << std::endl;
        messageFlow.emplace( std::make_pair(&sender,&vertex), messages.back() );
      }
    }
    if ( vertex.entry<BPMN::MessageThrowEvent>() ) {
      messageSenders.push_back(&vertex);
    }
  }
}

void CPModel::createMessagingConstraints() {
  for ( auto recipient : messageRecipients ) {
    assert( recipient->exit<BPMN::MessageCatchEvent>() );
    CP::Expression messagesDelivered(0);

    for ( Vertex& sender : recipient->senders ) {
      assert( sender.entry<BPMN::MessageThrowEvent>() );

      auto& message = messageFlow.at({&sender,recipient});
      messagesDelivered = messagesDelivered + message;
      
      model.addConstraint( message <= visit.at(recipient) );
      model.addConstraint( message <= visit.at(&sender) );
      
      model.addConstraint( message.implies( position.at(&sender) < position.at(recipient) ) );

      // this should not be necessary as status variables are respectively deduced
      model.addConstraint(
        // if a message is sent from a sender to a recipient, the recipient's timestamp must not 
        // be before the sender's timestamp
        message.implies (
          status.at(&sender)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          <= status.at(recipient)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        )
      );

      if ( sender.node->represents<BPMN::SendTask>() ) {
        // exit at send task must be after message delivery
        auto& recipientStatus = 
          ( recipient->node->represents<BPMN::ReceiveTask>() || recipient->node->represents<BPMN::MessageStartEvent>() ) ?
          std::get<0>(locals.at(recipient)[0]) : 
          status.at(recipient)
        ;          
        model.addConstraint(
          message.implies (
            status.at(exit(&sender))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
            >=
            recipientStatus[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          )
        );
      }

      // add message header constraints
      assert( messageHeader.contains(entry(recipient)) );
      assert( messageHeader.contains(&sender) );
      auto& recipientHeader = messageHeader.at(entry(recipient));
      auto& senderHeader = messageHeader.at(&sender);
      for ( auto& [key, recipientHeaderVariables] : recipientHeader ) {
        if ( !senderHeader.contains(key) ) {
          throw std::runtime_error("CPModel: illegal message flow from '" + sender.node->id + "' to '" + recipient->node->id  + "'");
        }
        model.addConstraint( 
          ( message && recipientHeaderVariables.defined && senderHeader.at(key).defined ).implies (
            recipientHeaderVariables.value == senderHeader.at(key).value 
          ) 
        );
      }      
      
      
      // add message content constraints
      auto& recipientContent = messageContent.at(recipient);
      auto& senderContent = messageContent.at(&sender);
      for ( auto& [key, recipientContentVariables] : recipientContent ) {
        if ( !senderContent.contains(key) ) {
          throw std::runtime_error("CPModel: illegal message flow from '" + sender.node->id + "' to '" + recipient->node->id  + "'");
        }
        model.addConstraint( message.implies ( recipientContentVariables.defined == senderContent.at(key).defined ) );
        model.addConstraint( message.implies ( recipientContentVariables.value == senderContent.at(key).value ) );      
      }
    }

    // every visited message catch event must receive exactly one message
    model.addConstraint( visit.at(recipient) == messagesDelivered );
  }

  for ( auto sender : messageSenders ) {
    assert( sender->entry<BPMN::MessageThrowEvent>() );
    CP::Expression messagesDelivered(0);
    for ( Vertex& recipient : sender->recipients ) {
      assert( recipient.exit<BPMN::MessageCatchEvent>() );
      auto& message = messageFlow.at({sender,&recipient});
      messagesDelivered = messagesDelivered + message;
    }
    if ( sender->entry<BPMN::SendTask>() ) {
      // a message thrown at a send task must be delivered to at exactly one recipient
      model.addConstraint( visit.at(sender) == messagesDelivered );
    }
    else {
      // a message thrown at other message throw events can be delivered to at most one recipient
      model.addConstraint( visit.at(sender) >= messagesDelivered );
    }
  }
}

void CPModel::createMessageHeader(const Vertex* vertex) {
  auto  extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements );
  
  if ( extensionElements->messageDefinitions.size() > 1 ) {
    assert(!"Not yet implemented");
  }

  auto messageDefinition = extensionElements->messageDefinitions.size() == 1 ? extensionElements->messageDefinitions[0].get() : nullptr;
  assert( messageDefinition );
  auto& messageHeaderVariables = messageHeader[vertex];

  for (size_t i = 0; i < messageDefinition->header.size(); i++ ) {
    if ( i == BPMNOS::Model::MessageDefinition::Index::Name ) {
      // skip message name
      continue;
    }
    auto& header = messageDefinition->header[i];
    if ( 
      ( i == BPMNOS::Model::MessageDefinition::Index::Sender && vertex->entry<BPMN::MessageThrowEvent>() ) ||
      ( i == BPMNOS::Model::MessageDefinition::Index::Recipient && vertex->entry<BPMN::MessageCatchEvent>() )
    ) {
      // set sender or recipient instance
      messageHeaderVariables.emplace( 
        header, 
        AttributeVariables{
          model.addVariable(CP::Variable::Type::BOOLEAN, "header_defined_{" + vertex->shortReference() + "}," + header, true ), 
          model.addVariable(CP::Variable::Type::REAL, "header_value_{" + vertex->shortReference() + "}," + header, (double)vertex->instanceId )
        }
      );
      continue;
    }

//std::cerr << "header: " << header << "/" << messageDefinition->parameterMap.contains(header) << std::endl;
    if ( messageDefinition->parameterMap.contains(header) ) {
      const BPMNOS::Model::Attribute* attribute = 
        messageDefinition->parameterMap.at(header)->expression ? 
        messageDefinition->parameterMap.at(header)->expression->isAttribute() :
        nullptr
      ;
      if ( attribute ) {
        auto [defined,value] = getAttributeVariables(vertex, attribute);
        messageHeaderVariables.emplace( 
          header, 
          AttributeVariables{
            model.addVariable(CP::Variable::Type::BOOLEAN, "header_defined_{" + vertex->shortReference() + "}," + header, defined ), 
            model.addVariable(CP::Variable::Type::REAL, "header_value_{" + vertex->shortReference() + "}," + header, value )
          }
        );
        continue;
      }
      
      if ( messageDefinition->parameterMap.at(header)->expression ) {
        auto headerValue = createExpression( vertex, *messageDefinition->parameterMap.at(header)->expression );
        messageHeaderVariables.emplace( 
          header, 
          AttributeVariables{
            model.addVariable(CP::Variable::Type::BOOLEAN, "header_defined_{" + vertex->shortReference() + "}," + header, visit.at(vertex) ), 
            model.addVariable(CP::Variable::Type::REAL, "header_value_{" + vertex->shortReference() + "}," + header, visit.at(vertex) * headerValue )
          }
        );
        continue;
      }
    }
    else {
      messageHeaderVariables.emplace( 
        header, 
        AttributeVariables{
          model.addVariable(CP::Variable::Type::BOOLEAN, "header_defined_{" + vertex->shortReference() + "}," + header, false ), 
          model.addVariable(CP::Variable::Type::REAL, "header_value_{" + vertex->shortReference() + "}," + header, 0.0 )
        }
      );
    }
  }
}

void CPModel::createMessageContent(const Vertex* vertex) {
  auto  extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  if ( extensionElements->messageDefinitions.size() > 1 ) {
    assert(!"Not yet implemented");
  }

  auto messageDefinition = extensionElements->messageDefinitions.size() == 1 ? extensionElements->messageDefinitions[0].get() : nullptr;
  assert( messageDefinition );
  auto& messageContentVariables = messageContent[vertex];

  if ( vertex->entry<BPMN::MessageThrowEvent>() ) {
    // deduce message content variables from status
    for (auto& [key,content] : messageDefinition->contentMap ) {
      auto [defined,value] = getAttributeVariables(vertex, content->attribute);
      messageContentVariables.emplace( 
        content->key, 
        AttributeVariables{
          model.addVariable(CP::Variable::Type::BOOLEAN, "content_defined_{" + vertex->shortReference() + "}," + content->key, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "content_value_{" + vertex->shortReference() + "}," + content->key, value )
        }
      );
    }
  }
  else {
    // create variables for the message content (relevant constraints must be added separately)
    for (auto& [key,content] : messageDefinition->contentMap ) {
      messageContentVariables.emplace( 
        content->key, 
        AttributeVariables{
          model.addBinaryVariable("content_defined_{" + vertex->shortReference() + "}," + content->key ), 
          model.addRealVariable("content_value_{" + vertex->shortReference() + "}," + content->key )
        }
      );
    }
  }
}

void CPModel::createDataVariables(const FlattenedGraph::Vertex* vertex) {
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  for ( auto& attribute : extensionElements->data ) {
    IndexedAttributeVariables variables( 
      model.addIndexedVariables(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + "}," + attribute->id ),
      model.addIndexedVariables(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + "}," + attribute->id )
    );
    // add variables holding initial values
    auto given = scenario.getKnownValue(vertex->rootId, attribute.get(), scenario.getInception());
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
    addToObjective( attribute.get(), variables.value[ variables.value.size() -1 ] );
    data.emplace( std::make_pair(vertex, attribute.get()), std::move(variables) ); 
  }
}

void CPModel::constrainDataVariables(const FlattenedGraph::Vertex* vertex) {
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  if ( extensionElements->data.empty() ) return;
  
//std::cerr << "modifiers of " << vertex->reference() << std::endl;
  auto& dataModifiers = flattenedGraph.dataModifiers.at(vertex);
  for ( auto& [entry,exit] : dataModifiers ) {
//std::cerr << "modifier: " << entry.reference() << std::endl;
    auto& [localStatus,localData,localGlobals] = locals.at(&exit).back();
    for ( unsigned int i = 0; i < dataModifiers.size(); i++ ) {
      for ( auto& attribute : extensionElements->data ) {
        if ( entry.node->represents<BPMN::TypedStartEvent>() || entry.node->represents<BPMN::ReceiveTask>() || entry.node->represents<BPMNOS::Model::DecisionTask>() ) {
          // operators are applied after message delivery or choice
          // ASSUMPTION: exit decision immediately follows message delivery or choice
          auto& index = dataIndex.at(&exit)[ exit.dataOwnerIndex(attribute.get()) ];
          // if global index at modifier exit equals i then the i-th global item equals the final globals of the modifier
          model.addConstraint( 
            ( index == i ).implies( 
              data.at({vertex,attribute.get()}).defined[i] == localData[attribute->index].defined
            ) 
          );
          model.addConstraint( 
            ( index == i ).implies( 
              data.at({vertex,attribute.get()}).value[i] == localData[attribute->index].value
            ) 
          );
        }
        else {
          assert( entry.node->represents<BPMN::Task>() );
          // operators are applied upon entry
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
}

void CPModel::constrainGlobalVariables() {
  for ( auto& [entry,exit] : flattenedGraph.globalModifiers ) {
    auto& [localStatus,localData,localGlobals] = locals.at(&exit).back();
    for ( unsigned int i = 0; i < flattenedGraph.globalModifiers.size(); i++ ) {
      for ( auto attribute : scenario.model->attributeRegistry.globalAttributes ) {
        if ( entry.node->represents<BPMN::TypedStartEvent>() || entry.node->represents<BPMN::ReceiveTask>() || entry.node->represents<BPMNOS::Model::DecisionTask>() ) {
          // operators are applied after message delivery or choice
          // ASSUMPTION: exit decision immediately follows message delivery or choice
          // if global index at modifier exit equals i then the i-th global item equals the final globals of the modifier
          model.addConstraint( 
            ( globalIndex.at(&exit) == i && visit.at(&exit) ).implies( 
              globals[attribute->index].defined[i] == localGlobals[attribute->index].defined
            ) 
          );
          model.addConstraint( 
            ( globalIndex.at(&exit) == i && visit.at(&exit) ).implies( 
              globals[attribute->index].value[i] == localGlobals[attribute->index].value
            ) 
          );
        }
        else {
          assert( entry.node->represents<BPMN::Task>() );
          // operators are applied upon entry
          // if global index at modifier entry equals i then the i+1-th global item equals the final globals of the modifier
          model.addConstraint( 
            ( globalIndex.at(&entry) == i && visit.at(&entry) ).implies( 
              globals[attribute->index].defined[i + 1] == localGlobals[attribute->index].defined
            ) 
          );
          model.addConstraint( 
            ( globalIndex.at(&entry) == i && visit.at(&entry) ).implies( 
              globals[attribute->index].value[i + 1] == localGlobals[attribute->index].value
            ) 
          );
        }
      }
    }
  }
}

void CPModel::constrainEventBasedGateway(const FlattenedGraph::Vertex* gateway) {
  assert( gateway->node->represents<BPMN::EventBasedGateway>() );
  // ensure a single outflow and determine all timers
  CP::Expression outflows(0);
  std::vector<CP::Expression> timers;
  for ( auto& [sequenceFlow,event] : gateway->outflows ) {
    assert( event.node->represents<BPMN::CatchEvent>() );
    outflows = outflows + tokenFlow.at({gateway,&event});
    if ( event.node->represents<BPMN::TimerCatchEvent>() ) {
      timers.push_back( createExpression(gateway,*event.node->extensionElements->as<BPMNOS::Model::Timer>()->trigger->expression) );
    }
  }
  model.addConstraint( outflows == visit.at(gateway) );
  
  // make sure that triggered event does not succeed any timer
  for ( auto& [sequenceFlow,event] : gateway->outflows ) {
    for ( auto& timer : timers ) {
      model.addConstraint( 
        visit.at(&event).implies(
          status.at(exit(&event))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value 
          <= 
          timer
//          CP::max(timer,status.at(entry(gateway))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value)
        ) 
      );
    }
  }
}

void CPModel::constrainSequentialActivities() {
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

void CPModel::createStatus(const Vertex* vertex) {
//std::cerr << "createStatus: " << vertex->reference() << std::endl;
  if ( vertex->type == Vertex::Type::ENTRY ) {
    createEntryStatus(vertex);
  }
  else {
    createExitStatus(vertex);
    addObjectiveCoefficients( vertex );
  }
}

void CPModel::addAttributes(const Vertex* vertex, std::vector<AttributeVariables>& variables, const BPMNOS::Model::Attribute* loopIndex) {
  // add new attributes if necessary
  if ( auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
    variables.reserve(extensionElements->attributeRegistry.statusAttributes.size());
    for ( size_t i = variables.size(); i < extensionElements->attributeRegistry.statusAttributes.size(); i++) {
      auto attribute = extensionElements->attributeRegistry.statusAttributes[i];
//std::cerr << "Add: " << attribute->id << std::endl;
      // add variables holding given values
      if ( auto given = scenario.getKnownValue(vertex->rootId, attribute, scenario.getInception()); given.has_value() ) {
        // defined initial value
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), (double)given.value(), 0.0)) 
        );
      }
      else if ( attribute == loopIndex ) {
//std::cerr << ( loopIndex ? loopIndex->id : "XXX") << " - Loop index: " << getLoopIndex(vertex).stringify() << std::endl;
        // set or increment loop index (initial value is assumed to be undefined, and therefore has a value of zero)
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL,"value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), getLoopIndex(vertex), 0 ) )
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

void CPModel::createEntryStatus(const Vertex* vertex) {
//std::cerr << "createEntryStatus: " << vertex->reference() << std::endl;
  status.emplace( vertex, std::vector<AttributeVariables>() );  
  auto& variables = status.at(vertex);
  auto loopCharacteristics = getLoopCharacteristics(vertex);

  if ( loopCharacteristics.has_value() && !flattenedGraph.dummies.contains(vertex) ) {
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
    assert( vertex->node->represents<BPMN::FlowNode>() );
    auto scope = vertex->node->as<BPMN::FlowNode>()->parent;
    assert( scope );
    auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert( extensionElements );
    if ( vertex->entry<BPMN::UntypedStartEvent>() ) {
      assert( vertex->parent.value().first.node == scope );
      assert( status.contains(&vertex->parent.value().first) );
      variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, status.at(&vertex->parent.value().first) );
    }
    else if ( vertex->entry<BPMN::TypedStartEvent>() ) {
      assert( scope->represents<BPMN::EventSubProcess>() );
      assert( vertex->parent.value().first.node == scope->as<BPMN::EventSubProcess>()->parent );
      assert( status.contains(&vertex->parent.value().first) );
      // use attribute registry of parent of event-subprocess scope
      auto& attributeRegistry = scope->as<BPMN::EventSubProcess>()->parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
      variables = createUniquelyDeducedEntryStatus(vertex, attributeRegistry, status.at(&vertex->parent.value().first) );
      // attributes defined for event-subprocess are added later
    }
    else if ( vertex->entry<BPMN::ExclusiveGateway>() && vertex->inflows.size() > 1 ) {
      std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives;
      for ( auto& [sequenceFlow,predecessor] : vertex->inflows ) {
        // add to alternatives
        assert( statusFlow.contains({&predecessor,vertex}) );
        alternatives.emplace_back( tokenFlow.at({&predecessor,vertex}), statusFlow.at({&predecessor,vertex}) );
      }
      variables = createAlternativeEntryStatus(vertex, extensionElements->attributeRegistry, std::move(alternatives));
    }
    else if ( vertex->entry<BPMN::FlowNode>() && vertex->inflows.size() > 1 ) {
      assert(vertex->entry<BPMN::ParallelGateway>() || vertex->entry<BPMN::InclusiveGateway>() );
      assert( vertex->predecessors.size() == 1 );
      if ( !vertex->entry<BPMN::ParallelGateway>() && !vertex->entry<BPMN::InclusiveGateway>() ) {
        throw std::runtime_error("CPModel: illegal join at '" + vertex->node->id + "'");
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
        assert( statusFlow.contains({&predecessor,vertex}) );
        variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, statusFlow.at({&predecessor,vertex}) );
      }
      else {
        assert( vertex->node->represents<BPMN::Activity>() );
        assert( vertex->node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );
        assert( status.contains(&predecessor) ); 
        variables = createUniquelyDeducedEntryStatus(vertex, extensionElements->attributeRegistry, status.at(&predecessor) );
      }
    }
  }

  if ( 
    flattenedGraph.dummies.contains(vertex) &&
    loopCharacteristics.has_value() &&
    loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
  ) {
    // for multi-instance activities, attributes are only added to vertices representing and instantiation 
    return;
  }
  // add new attributes where applicable
  addAttributes(vertex,variables);
}

void CPModel::createExitStatus(const Vertex* vertex) {
//std::cerr << "createExitStatus" << std::endl;
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
      throw std::runtime_error("CPModel: empty scopes are not supported");
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
        throw std::runtime_error("CPModel: unable to determine end nodes for scope '" + vertex->node->id  + "'");
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

//std::cerr << vertex->reference() << std::endl;  
  const Vertex* entryVertex = entry(vertex);
  if ( vertex->node->represents<BPMN::TimerCatchEvent>() ) {
//std::cerr << "Timer: " << vertex->reference() << std::endl;
    assert( vertex->node->extensionElements->represents<BPMNOS::Model::Timer>() );
    auto trigger = createExpression(entry(vertex),*vertex->node->extensionElements->as<BPMNOS::Model::Timer>()->trigger->expression);
    auto& entryStatus = status.at(entryVertex);
    std::vector<AttributeVariables> variables;
//std::cerr << "timer loop" << std::endl;
    extensionElements = vertex->parent.value().first.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
//std::cerr << attribute->name << ": " << attribute->index << " == " <<  variables.size() << std::endl;
//      assert( attribute->index == variables.size() );
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].defined * CP::max( entryStatus[attribute->index].value, trigger ) )
        );
      }
      else {
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].value )
        );
      }      
    }
    status.emplace( vertex, std::move(variables) );
    return;
  }
  else if ( !extensionElements ) {
    // token just runs through the node and exit status is the same as entry status
    extensionElements = vertex->parent.value().first.node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
//std::cerr << entryVertex->reference() << std::endl;  
    auto& entryStatus = status.at(entryVertex);
    std::vector<AttributeVariables> variables;
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
//std::cerr << attribute->name << ": " << attribute->index << " == " <<  variables.size() << std::endl;
//      assert( attribute->index == variables.size() );
      variables.emplace_back(
        model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].defined ), 
        model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, entryStatus[attribute->index].value )
      );      
    }
    status.emplace( vertex, std::move(variables) );
    return;
  }

  if ( vertex->exit<BPMN::Task>() || vertex->exit<BPMN::TypedStartEvent>() ) {
    // create local attribute variables considering all changes made before the token leves the node
    createLocalAttributeVariables(vertex);
    // use final attribute values to deduce exit status
    auto& [ localStatus, localData, localGlobals ] = locals.at(vertex).back();

    // create exit status
    std::vector<AttributeVariables> variables;
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
      auto& [ defined, value ] = localStatus.at(attribute->index);
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp && vertex->node->represents<BPMN::Task>() ) {
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

  // copy entry status references
  assert( status.contains(entryVertex) );
  std::vector<AttributeVariables> currentStatus = status.at(entryVertex);
  //std::vector<AttributeVariables> currentData = xxx.at(entryVertex);
  //std::vector<AttributeVariables> currentGlobal = xxx.at(entryVertex);
  
/*
  if ( vertex->node->represents<BPMNOS::Model::DecisionTask>() ) {
    // choices are represented by unconstrained status (data/global) attributes
    // TODO
      assert(!"Not yet implemented");
  }
*/
  if ( vertex->node->represents<BPMN::MessageCatchEvent>() ) {
    assert( extensionElements );

    if ( extensionElements->messageDefinitions.size() > 1 ) {
      assert(!"Not yet implemented");
    }

    auto messageDefinition = extensionElements->messageDefinitions.size() == 1 ? extensionElements->messageDefinitions[0].get() : nullptr;
    assert( messageContent.contains(vertex) ); 
    auto& messageContentVariables = messageContent.at(vertex);

    // create exit status
    std::vector<AttributeVariables> variables;
    auto& entryStatus = status.at( entry(vertex) );
    for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        // timestamp is implicitly determined as the time when the message is delivered
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addRealVariable("value_{" + vertex->reference() + "}," + attribute->id )
        );
      }
      else if ( auto content = findContent(messageDefinition, attribute) ) {
        auto& [ defined, value ] = messageContentVariables.at(content->key);
        if ( 
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
        ) {
          throw std::runtime_error("CPModel: timestamp must not be changed by message content");
        }
        // attribute set by received message content
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
        );
      }
      else {
        auto& [ defined, value ] = entryStatus.at(attribute->index);
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
        );
      }      
    }
    status.emplace( vertex, std::move(variables) );
    return;
  }
  
  if ( vertex->node->represents<BPMN::MessageStartEvent>() ) {
    // add entry restrictions of event-subprocess
    // TODO
  }

  std::vector<AttributeVariables> variables;
  for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
    assert( attribute->index == variables.size() );
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, currentStatus[attribute->index].defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, currentStatus[attribute->index].value )
    );      

    // ensure that attribute only has a value if vertex is visited and the attribute is defined
    assert( visit.contains(vertex) );
    model.addConstraint( variables[attribute->index].defined <= visit.at(vertex) );
    model.addConstraint( (!variables[attribute->index].defined).implies( variables[attribute->index].value == 0.0 ) );
  }

  status.emplace( vertex, std::move(variables) );
}

std::pair< CP::Expression, CP::Expression > CPModel::getLocalAttributeVariables( const Model::Attribute* attribute, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables ) {
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

CP::Expression CPModel::createOperatorExpression( const Model::Expression& operator_, std::tuple< std::vector<AttributeVariables>, std::vector<AttributeVariables>, std::vector<AttributeVariables> >& localVariables ) {
  auto compiled = LIMEX::Expression<CP::Expression,CP::Expression>(operator_.expression, limexHandle);
  
  std::vector<CP::Expression> variables;
  for ( auto& variableName : compiled.getVariables() ) {
    auto attribute = operator_.attributeRegistry[variableName];
    if( attribute->type == ValueType::COLLECTION ) {
      throw std::runtime_error("CPModel: illegal operator expression '" + operator_.expression + "'");
    }

    auto [defined,value] = getLocalAttributeVariables(attribute,localVariables);
    variables.push_back( value );
  }
  
  std::vector<CP::Expression> collectionVariables;
  for ( auto& variableName : compiled.getCollections() ) {
    auto attribute = operator_.attributeRegistry[variableName];
    if( attribute->type != ValueType::COLLECTION ) {
      throw std::runtime_error("CPModel: illegal operator expression '" + operator_.expression + "'");
    }

    auto [defined,value] = getLocalAttributeVariables(attribute,localVariables);
    collectionVariables.push_back( value );
  }
  
  return compiled.evaluate(variables,collectionVariables);
}


const BPMNOS::Model::Choice* CPModel::findChoice(const std::vector< std::unique_ptr<BPMNOS::Model::Choice> >& choices, const BPMNOS::Model::Attribute* attribute) const {
//std::cerr << "choices: " << choices.size() << "/" << attribute->id << std::endl;
  if ( choices.empty() ) return nullptr;
  auto it = std::find_if(
    choices.begin(),
    choices.end(),
    [&](const auto& choice) {
//std::cerr << "check: " << choice->attribute->id << std::endl;
      return choice->attribute == attribute;
    }
  ); 
  return ( it != choices.end() ? it->get() : nullptr );
}

const BPMNOS::Model::Content* CPModel::findContent(const BPMNOS::Model::MessageDefinition* messageDefinition, const BPMNOS::Model::Attribute* attribute) const {
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
}


void CPModel::createLocalAttributeVariables(const Vertex* vertex) {
//std::cerr << "createLocalAttributeVariables" << std::endl;
  assert( vertex->exit<BPMN::Task>() || vertex->exit<BPMN::TypedStartEvent>() );
  
  if ( auto typedStartEvent = vertex->node->represents<BPMN::TypedStartEvent>();
    typedStartEvent && !typedStartEvent->represents<BPMN::MessageStartEvent>()
  ) {
    throw std::runtime_error("CPModel: illegal typed start event");
  }
  auto  extensionElements = 
    vertex->exit<BPMN::Task>() ?
    vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>() :
    vertex->node->as<BPMN::TypedStartEvent>()->parent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  ;

  auto& messageDefinitions = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions;
  if ( messageDefinitions.size() > 1 ) {
    assert(!"Not yet implemented");
  }

  auto messageDefinition = 
    ( vertex->exit<BPMN::MessageCatchEvent>() && messageDefinitions.size() == 1 ) ? 
    messageDefinitions[0].get() : 
    nullptr
  ;

  auto localAttributeVariables = [&](const auto& attributes) -> std::vector<AttributeVariables> {
    std::vector<AttributeVariables> variables;
//std::cerr << "Attributes: " << attributes.size() << std::endl;

    auto initialVariables = [&](BPMNOS::Model::Attribute* attribute) -> std::pair<CP::Expression,CP::Expression> {
      auto entryVertex = entry(vertex);
      if ( attribute->category == BPMNOS::Model::Attribute::Category::STATUS ) {
        return std::make_pair<CP::Expression,CP::Expression>( 
          status.at(entryVertex)[attribute->index].defined, 
          status.at(entryVertex)[attribute->index].value
        );
      }
      else if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        auto& index = dataIndex.at(entryVertex)[ entryVertex->dataOwnerIndex(attribute) ];
        return std::make_pair<CP::Expression,CP::Expression>( 
          data.at( {&entryVertex->dataOwner(attribute).first, attribute } ).defined[ index ], 
          data.at( {&entryVertex->dataOwner(attribute).first, attribute } ).value[ index ]
        );
      }
      else {
        assert( attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL );
        auto& index = globalIndex.at(entryVertex);
        return std::make_pair<CP::Expression,CP::Expression>( 
          globals[attribute->index].defined[ index ],
          globals[attribute->index].value[ index ] 
        );
      }
    };

    for ( auto attribute : attributes ) {
//std::cerr << "Attribute: " << attribute->id << std::endl;
      auto [defined,value] = initialVariables(attribute);
    
      if ( auto choice = findChoice(extensionElements->choices, attribute) ) {
        if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
        ) {
          throw std::runtime_error("CPModel: timestamp must not be a choice");
        }

        // attribute set by choice
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, visit.at(vertex) ), 
          model.addRealVariable("value_{" + vertex->shortReference() + ",0}," + attribute->id )
        );
        auto& variable = variables.back().value;
        auto timestamp = extensionElements->attributeRegistry.statusAttributes[BPMNOS::Model::ExtensionElements::Index::Timestamp];
        if ( choice->dependencies.contains(timestamp) ) {
          throw std::runtime_error("CPModel: choices depending on the timestamp are not supported");
        }
        // create constraints limiting the choice
        // ASSUMPTION: constraints on the choices must only depend on entry status, data, or globals.
        // ASSUMPTION: only status attributes are allowed as choices
        auto [ strictLB, strictUB ] = choice->strictness;
        if ( choice->lowerBound.has_value() ) {
          if ( strictLB ) {
            model.addConstraint( visit.at(vertex).implies( variable > createExpression(entry(vertex),choice->lowerBound.value()) ) );
          }
          else {
            model.addConstraint( visit.at(vertex).implies( variable >= createExpression(entry(vertex),choice->lowerBound.value()) ) );
          }
        }
        if ( choice->upperBound.has_value() ) {
          if ( strictUB ) {
            model.addConstraint( visit.at(vertex).implies( variable < createExpression(entry(vertex),choice->upperBound.value()) ) );
          }
          else {
            model.addConstraint( visit.at(vertex).implies( variable <= createExpression(entry(vertex),choice->upperBound.value()) ) );
          }
        }
        if ( !choice->enumeration.empty() ) {
          CP::Expression enumerationContainsVariable(false);
          for ( auto expression : choice->enumeration ) {
            enumerationContainsVariable = enumerationContainsVariable || ( variable == createExpression(entry(vertex),expression) );
          }
          model.addConstraint( visit.at(vertex).implies( enumerationContainsVariable ) );
        }        
//        assert(!"Not yet implemented");
      }
      else if ( auto content = findContent(messageDefinition, attribute) ) {
//std::cerr << "Content: " << content->key << std::endl;
        if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp
        ) {
          throw std::runtime_error("CPModel: timestamp must not be changed by message content");
        }
        assert(messageContent.contains(vertex));
        assert(messageContent.at(vertex).contains(content->key));
        // attribute set by received message content
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, messageContent.at(vertex).at(content->key).defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + ",0}," + attribute->id, messageContent.at(vertex).at(content->key).value )
        );
      }
      else if ( 
          attribute->category == BPMNOS::Model::Attribute::Category::STATUS &&
          attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp &&
          ( vertex->exit<BPMN::MessageCatchEvent>() || vertex->exit<BPMNOS::Model::DecisionTask>() )
      ) {
          // timestamp cannot be deduced as must be constrained
          variables.emplace_back(
            model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, visit.at(vertex) ), 
            model.addRealVariable("value_{" + vertex->shortReference() + ",0}," + attribute->id )
          );
          auto& timestamp = variables.back().value;
          model.addConstraint( visit.at(vertex).implies( timestamp >= status.at(entry(vertex))[BPMNOS::Model::ExtensionElements::Index::Timestamp].value ) );
      }
      else {
        // all local attribute variables are deduced from initial variables upon entry
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->shortReference() + ",0}," + attribute->id, defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->shortReference() + ",0}," + attribute->id, value )
        );
      }
//std::cerr << "Attribute: " << attribute->id << " (done)" << std::endl;
    }
    
    return variables;
  }; // end of lambda
  
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
            throw std::runtime_error("CPModel: illegal operator expression: " + operator_->expression.expression );
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

std::vector<CPModel::AttributeVariables> CPModel::createUniquelyDeducedEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector<AttributeVariables>& inheritedStatus) {
//std::cerr << vertex->reference() << std::endl;
  assert( vertex->type == Vertex::Type::ENTRY );
  assert( !vertex->node->represents<BPMN::Process>() );

  std::vector<AttributeVariables> variables;
  variables.reserve( attributeRegistry.statusAttributes.size() );
  for ( auto attribute : attributeRegistry.statusAttributes ) {
//std::cerr << variables.size() << "/" << attribute->index << "/" << attribute->id << "/" << attributeRegistry.statusAttributes.size() << std::endl;
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
    else if ( vertex->entry<BPMN::TypedStartEvent>() ) {
      if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        CP::Expression timestamp(0);
        for ( Vertex& predecessor : vertex->predecessors ) {
          timestamp = CP::max( timestamp, status.at(&predecessor)[attribute->index].value );
        }
        // deduce variable if visited
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) * timestamp )
        );
      }
      else {
        // deduce variable if visited
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) * defined ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) * value )
        );
      }
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


std::vector<CPModel::AttributeVariables> CPModel::createAlternativeEntryStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > alternatives) {
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
      value = value + attributeVariables[attribute->index].defined * attributeVariables[attribute->index].value;
    }
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, defined ), 
      model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, value )
    );
  }

  return variables;
}


std::vector<CPModel::AttributeVariables> CPModel::createMergedStatus(const Vertex* vertex, const BPMNOS::Model::AttributeRegistry& attributeRegistry, std::vector< std::pair<const CP::Variable&, std::vector<AttributeVariables>& > > inputs) {
//  assert( ( vertex->type == Vertex::Type::ENTRY && vertex->inflows.size() > 1) || vertex->exit<BPMN::Scope>() );
//  assert( !vertex->entry<BPMN::Activity>() );
  std::vector<AttributeVariables> variables;
  variables.reserve( attributeRegistry.statusAttributes.size() );
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      std::vector<CP::Expression> terms;
      assert( !inputs.empty() );
      for ( auto& [ active, attributeVariables] : inputs ) {
        terms.emplace_back( attributeVariables[attribute->index].defined * attributeVariables[attribute->index].value );
      }
      if ( vertex->node->represents<BPMN::Activity>() && !flattenedGraph.dummies.contains(vertex) ) {
        // timestamp of vertex must be at least the maximum timestamp of all inputs
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addRealVariable("value_{" + vertex->reference() + "}," + attribute->id )
        );
        model.addConstraint( visit.at(vertex).implies( variables.back().value >= CP::max(terms) ) );
      }
      else {
        // timestamp of vertex is the same as the maximum timestamp of all inputs
        variables.emplace_back(
          model.addVariable(CP::Variable::Type::BOOLEAN, "defined_{" + vertex->reference() + "}," + attribute->id, visit.at(vertex) ), 
          model.addVariable(CP::Variable::Type::REAL, "value_{" + vertex->reference() + "}," + attribute->id, CP::if_then_else( visit.at(vertex), CP::max(terms), 0.0 ) )
        );
      }
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

CP::Expression CPModel::getLoopIndex(const Vertex* vertex) {
  auto predecessor = &vertex->inflows.front().second;
  if ( getLoopCharacteristics(vertex).value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    const BPMNOS::Model::Attribute* attribute = 
      extensionElements->loopIndex.has_value() ? 
      extensionElements->loopIndex.value()->expression->isAttribute() :
      nullptr
    ;
    if ( attribute->index >= status.at(predecessor).size() ) {
      return 1;
    }
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

void CPModel::createLoopEntryStatus(const Vertex* vertex) {
//std::cerr << "createLoopEntryStatus " << vertex->reference() << std::endl;
  assert( vertex->inflows.size() == 1 );
  assert( status.contains(vertex) );
  auto& variables = status.at(vertex);
  auto& predecessor = vertex->inflows.front().second;
  auto& priorStatus = status.at(&predecessor);
//std::cerr << predecessor.reference() << "/" << priorStatus.size() << std::endl;
  auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  const BPMNOS::Model::Attribute* loopIndex = 
    extensionElements->loopIndex.has_value() ? 
    extensionElements->loopIndex.value()->expression->isAttribute() :
    nullptr
  ;

  // deduce status for loop visit 
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  
  for ( size_t index = 0; index < priorStatus.size(); index++ ) {
    auto attribute = extensionElements->attributeRegistry.statusAttributes[index];
    assert( variables.size() == attribute->index );
//std::cerr << attribute->id << std::endl;
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

  if ( priorStatus.size() < extensionElements->attributeRegistry.statusAttributes.size() ) {
    addAttributes(vertex,variables,loopIndex);
  }

}

std::vector<CPModel::AttributeVariables> CPModel::createLoopExitStatus(const Vertex* vertex) {
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

void CPModel::createVertexVariables(const FlattenedGraph::Vertex* vertex) {
//std::cerr << "createVertexVariables: " << vertex->reference() << std::endl;
 
//std::cerr << "createGlobalIndexVariable" << std::endl;
  createGlobalIndexVariable(vertex);
//std::cerr << "createDataIndexVariables" << std::endl;
  createDataIndexVariables(vertex);
  
  if ( vertex->type == Vertex::Type::ENTRY ) {
//std::cerr << "createEntryVariables" << std::endl;
    createEntryVariables(vertex);
  }

//std::cerr << "createDataVariables: " << vertex->reference() << std::endl;
  if ( vertex->entry<BPMN::Scope>() ) {
    createDataVariables(vertex);
  }

  if ( vertex->exit<BPMN::MessageCatchEvent>() ) {
//std::cerr << "createMessageContent" << std::endl;
    createMessageContent(vertex);
  }

//std::cerr << "createStatus" << std::endl;
  createStatus(vertex);

  if ( vertex->entry<BPMN::MessageThrowEvent>() || vertex->entry<BPMN::MessageCatchEvent>() ) {
//std::cerr << "createMessageHeader" << std::endl;
    createMessageHeader(vertex);
  }
  
  if ( vertex->entry<BPMN::MessageThrowEvent>() ) {
//std::cerr << "createMessageContent" << std::endl;
    createMessageContent(vertex);
  }

  if ( vertex->type == Vertex::Type::EXIT ) {
    createExitVariables(vertex);
  }
    
//std::cerr << "createSequenceConstraints" << std::endl;
  createSequenceConstraints(vertex);

//std::cerr << "createRestrictions" << std::endl;
  createRestrictions(vertex);

//std::cerr << "Done(createVertexVariables)" << std::endl;
}

std::pair< CP::Expression, CP::Expression > CPModel::getAttributeVariables( const Vertex* vertex, const Model::Attribute* attribute) {
  if ( attribute->category == Model::Attribute::Category::STATUS ) {
    assert( status.contains(vertex) );
    assert( attribute->index < status.at(vertex).size() );
    return std::make_pair<CP::Expression,CP::Expression>( 
      status.at(vertex)[attribute->index].defined,
      status.at(vertex)[attribute->index].value 
    );
  }
  else if ( attribute->category == Model::Attribute::Category::DATA ) {
    if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Instance ) {
      return std::make_pair<CP::Expression,CP::Expression>( 
        true,
        (double)vertex->instanceId
      );
    }
    assert( data.contains( {&vertex->dataOwner(attribute).first, attribute } ) );
    if ( attribute->isImmutable ) {
      return std::make_pair<CP::Expression,CP::Expression>( 
        data.at( {&vertex->dataOwner(attribute).first, attribute } ).defined[ 0 ],
        data.at( {&vertex->dataOwner(attribute).first, attribute } ).value[ 0 ]
      );
    }
    else {
      auto& index = dataIndex.at(vertex)[ vertex->dataOwnerIndex(attribute) ];
      return std::make_pair<CP::Expression,CP::Expression>( 
        data.at( {&vertex->dataOwner(attribute).first, attribute } ).defined[ index ],
        data.at( {&vertex->dataOwner(attribute).first, attribute } ).value[ index ]
      );
    }
  }
  else {
    assert( attribute->category == Model::Attribute::Category::GLOBAL );
    assert( globals.size() > attribute->index );
    if ( attribute->isImmutable ) {
      return std::make_pair<CP::Expression,CP::Expression>( 
        globals[attribute->index].defined[ 0 ],
        globals[attribute->index].value[ 0 ] 
      );
    }
    else {
      auto& index = globalIndex.at(vertex);
      return std::make_pair<CP::Expression,CP::Expression>( 
        globals[attribute->index].defined[ index ],
        globals[attribute->index].value[ index ] 
      );
    }
  }
}

void CPModel::createEntryVariables(const FlattenedGraph::Vertex* vertex) {
//std::cerr << "createEntryVariables: " << vertex->reference() << std::endl;      
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
  else if ( auto typedStartEvent = vertex->node->represents<BPMN::TypedStartEvent>() ) {
    if ( 
      !typedStartEvent->parent->represents<BPMN::EventSubProcess>() ||
      typedStartEvent->isInterrupting ||
      !typedStartEvent->represents<BPMN::MessageStartEvent>() 
    ) {
      throw std::runtime_error("CPModel: typed start event '" + typedStartEvent->id + "' is not supported");
    }
    // deduce visit from incoming message flow
    CP::Expression messageDelivered(false);
    for ( Vertex& sender : exit(vertex)->senders ) {
      assert( messageFlow.contains({&sender,exit(vertex)}) );
      messageDelivered = messageDelivered || messageFlow.at({&sender,exit(vertex)});
    }    
    auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}", messageDelivered );
    visit.emplace(vertex, deducedVisit );
    visit.emplace(exit(vertex), deducedVisit );
    // event-subprocess may only be triggered if all predecessors are visited
    for ( Vertex& predecessor : vertex->predecessors ) {
      model.addConstraint( visit.at(vertex) <= visit.at(&predecessor) );
    }
  }
  else if ( vertex->node->represents<BPMN::FlowNode>() ) {
//std::cerr << vertex->reference() << ": " <<  vertex->loopIndices.size()  << "/" << flattenedGraph.loopIndexAttributes.at(vertex->node).size() << "/" << getLoopCharacteristics(vertex).has_value() << "/" << flattenedGraph.dummies.contains(vertex) << std::endl;      
    if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
      loopCharacteristics.has_value() &&
      !flattenedGraph.dummies.contains(vertex)    
    ) {
//std::cerr << "Loop/MI" << std::endl;
      if ( loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
        // loop vertex
        auto predecessor = &vertex->inflows.front().second;
        if ( flattenedGraph.dummies.contains(predecessor) ) {
          // first loop vertex
          auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) );
          visit.emplace(vertex, deducedVisit );
          visit.emplace(exit(vertex), deducedVisit );
//std::cerr << deducedVisit.stringify() << std::endl;
        }
        else {
          auto extensionElements = vertex->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
          if ( extensionElements->loopCondition.has_value() ) {
            // vertex is visited only if predecessor is visited and loop condition is satisifed
            auto condition = createExpression(predecessor, *extensionElements->loopCondition.value()->expression);
            auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , visit.at( predecessor ) && condition );
            visit.emplace(vertex, deducedVisit );
            visit.emplace(exit(vertex), deducedVisit );
//std::cerr << deducedVisit.stringify() << std::endl;
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
//std::cerr << "multi-instance vertex" << std::endl;
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
//std::cerr << "SINGLE INFLOW" << std::endl;
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
    else if ( vertex->inflows.size() > 1 ) {
//std::cerr << vertex->reference() << std::endl;
      if( !vertex->node->represents<BPMN::Gateway>() ) {
        throw std::runtime_error("CPModel: converging gateways must be explicit");
      }
      else {
        // deduce visit from incoming token flows
        CP::Expression incomingToken(false);
        for ( auto& [sequenceFlow, predecessor] : vertex->inflows ) {
          incomingToken = incomingToken || tokenFlow.at({&predecessor,vertex});
        }
        auto& deducedVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}" , incomingToken );
        visit.emplace(vertex, deducedVisit );
        visit.emplace(exit(vertex), deducedVisit );
//std::cerr <<  deducedVisit.stringify() << std::endl;
//        assert(!"Not yet implemented");
      }
    }
    else {
      assert( vertex->inflows.empty() );
//std::cerr << vertex->jsonify().dump() << std::endl;
      throw std::logic_error("CPModel: Every vertex except process entry should have inflow!");
//      assert(!"Not yet implemented");
    }
  }
  else if ( vertex->node->represents<BPMN::Process>() ) {
    // every process vertex is visited
    auto& knownVisit = model.addVariable(CP::Variable::Type::BOOLEAN, "visit_{" + vertex->shortReference() + "}", true, true );
//std::cerr << vertex << "//" << exit(vertex) << std::endl;
    visit.emplace(vertex, knownVisit );
    visit.emplace(exit(vertex), knownVisit );
  }
}

void CPModel::createExitVariables(const Vertex* vertex) {
//std::cerr << "createExitVariables" << std::endl;
  if ( vertex->node->represents<BPMN::FlowNode>() ) {
    if ( auto loopCharacteristics = getLoopCharacteristics(vertex);
      loopCharacteristics.has_value() &&
      loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard
    ) {
      if ( vertex->loopIndices.size() == flattenedGraph.loopIndexAttributes.at(vertex->node).size() ) {
        return;
      }
    }

    // flow variables
    if ( vertex->outflows.size() == 1 ) {
      auto& [sequenceFlow,target] = vertex->outflows.front();
      if ( sequenceFlow ) {
        createSequenceFlowVariables( vertex, &target );
      }
    }
    else if ( vertex->outflows.size() > 1 ) {
//std::cerr << vertex->reference() << std::endl;
      if( !vertex->node->represents<BPMN::Gateway>() ) {
        throw std::runtime_error("CPModel: diverging gateways must be explicit");
      }
      else if( vertex->node->represents<BPMN::EventBasedGateway>() ) {
        // create exclusive outflow
        for ( auto& [sequenceFlow,target] : vertex->outflows ) {
          tokenFlow.emplace( 
            std::make_pair(vertex,&target), 
            model.addBinaryVariable("tokenflow_{" + vertex->reference() + " → " + target.reference() + "}" ) 
          );
          createStatusFlowVariables(vertex,&target);
        }
      }
      else if( vertex->node->represents<BPMN::ComplexGateway>() ) {
        throw std::runtime_error("CPModel: complex gateways are not supported");
      }
      else if( vertex->node->represents<BPMN::ParallelGateway>() ) {
        // create unconditional outflow
        for ( auto& [sequenceFlow,target] : vertex->outflows ) {
          assert( sequenceFlow );
          createSequenceFlowVariables( vertex, &target );
        }
      }
      else {
        // create conditional outflow
        for ( auto& [sequenceFlow,target] : vertex->outflows ) {
          assert( sequenceFlow );
          createSequenceFlowVariables( vertex, &target, sequenceFlow->extensionElements->as<BPMNOS::Model::Gatekeeper>() );
        }
        if( vertex->node->represents<BPMN::ExclusiveGateway>() ) {
          CP::Expression outflows(0);
          for ( auto& [sequenceFlow,target] : vertex->outflows ) {
            outflows = outflows + tokenFlow.at({vertex,&target});
          }
          model.addConstraint( outflows == visit.at(vertex) );
        }
      }
    }
  }
//std::cerr << "Done(exitvariables)" << std::endl;
}

void CPModel::createSequenceFlowVariables(const Vertex* source, const Vertex* target, const BPMNOS::Model::Gatekeeper* gatekeeper) {
  if ( gatekeeper ) {
    CP::Expression gatekeeperCondition(true);
    for ( auto& condition : gatekeeper->conditions ) {
      gatekeeperCondition = gatekeeperCondition && createExpression(source,condition->expression);
    }
    tokenFlow.emplace( 
      std::make_pair(source,target), 
      model.addVariable(CP::Variable::Type::BOOLEAN, "tokenflow_{" + source->reference() + " → " + target->reference() + "}", visit.at(source) && gatekeeperCondition ) 
    );
  }
  else {
    tokenFlow.emplace( 
      std::make_pair(source,target), 
      model.addVariable(CP::Variable::Type::BOOLEAN, "tokenflow_{" + source->reference() + " → " + target->reference() + "}", visit.at(source) ) 
    );
  }
//std::cerr << "tokenFlow: " << source->reference() << " to " << target->reference() << std::endl;
  createStatusFlowVariables(source,target);
}

void CPModel::createStatusFlowVariables(const Vertex* source, const Vertex* target) {
//std::cerr << "statusFlow: " << source->reference() << " to " << target->reference() << std::endl;
  assert( source->node->represents<BPMN::FlowNode>() );
  assert( source->node->as<BPMN::FlowNode>()->parent );
  auto extensionElements = source->node->as<BPMN::FlowNode>()->parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements );
  std::vector<AttributeVariables> variables;
  variables.reserve( extensionElements->attributeRegistry.statusAttributes.size() );
  for ( auto attribute : extensionElements->attributeRegistry.statusAttributes ) {
//std::cerr << "statusAttribute: " << attribute->id << "/" << source->reference() << std::endl;
    assert( tokenFlow.contains({source,target}) );
    assert( status.contains(source) );
//std::cerr << "indices: " << attribute->index << " < " << status.at(source).size() << std::endl;
    assert( attribute->index < status.at(source).size() );
    // deduce variable
    variables.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "statusflow_defined_{" + source->reference() + " → " + target->reference() + "}," + attribute->id, 
        CP::if_then_else( 
          tokenFlow.at({source,target}), 
          status.at(source)[attribute->index].defined, 
          false
        )
      ), 
      model.addVariable(CP::Variable::Type::REAL, "statusflow_value_{" + source->reference() + " → " + target->reference() + "}," + attribute->id, 
        CP::if_then_else( 
          tokenFlow.at({source,target}), 
          status.at(source)[attribute->index].value, 
          0.0
        )
      )
    );
  }      

  statusFlow.emplace( 
    std::make_pair(source,target), 
    std::move(variables)
  );
}

void CPModel::createSequenceConstraints(const Vertex* vertex) {
  auto addConstraints = [&](const Vertex* predecessor) {
    assert( position.contains(predecessor) );
    assert( position.contains(vertex) );
    assert( visit.contains(vertex) );
    assert( status.contains(predecessor) );
    assert( status.at(predecessor).size() >= BPMNOS::Model::ExtensionElements::Index::Timestamp );
    assert( status.contains(vertex) );
    assert( status.at(vertex).size() >= BPMNOS::Model::ExtensionElements::Index::Timestamp );
    if ( vertex->exit<BPMN::TypedStartEvent>() ) {
      model.addConstraint( position.at(vertex) == position.at(entry(vertex)) + 1 );
    }
    else {
      model.addConstraint( position.at(predecessor) < position.at(vertex) );
    }
/*
std::cerr << predecessor->reference() << " before " << vertex->reference()  << std::endl;  
std::cerr << visit.at(vertex).stringify() << "\n implies \n";
std::cerr << status.at(predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value.stringify() << "\n <= \n";
std::cerr << status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value.stringify() << std::endl;
*/  
    if ( predecessor->node == vertex->node ) {  
      model.addConstraint(
        // predecessor and vertex are at the same node
        ( visit.at(vertex) ).implies(
          status.at(predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          <= status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        )
      );
    }
    else {
      model.addConstraint(
        // predecessor and vertex are at different nodes
        ( visit.at(predecessor) && visit.at(vertex) ).implies( 
          status.at(predecessor)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
          <= status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
        )
      );
    }

//std::cerr << model.getConstraints().back().stringify() << std::endl;
  };

  for ( auto& [_, predecessor] : vertex->inflows ) {
    addConstraints(&predecessor);
  }

  for ( const Vertex& predecessor : vertex->predecessors ) {
    addConstraints(&predecessor);
  }
}

void CPModel::constrainTypedStartEvent(const FlattenedGraph::Vertex* startEvent) {
//std::cerr << "TypedStartEvent " << startEvent->shortReference() << std::endl;        
  Vertex& parentExit = startEvent->parent.value().second;
  // the event-subprocess can be triggered until the last token of the parent is consumed
  for ( auto& [_, predecessor] : parentExit.inflows ) {
    if ( predecessor.node->as<BPMN::FlowNode>()->parent == parentExit.node ) {
      assert(visit.contains(startEvent));
//std::cerr << predecessor.reference() << std::endl;        
      assert(visit.contains(&predecessor));
      // if the event-subprocess is not triggered, then the position must be after the last visited end node
//auto& constraint =
      model.addConstraint(
        ( !visit.at(startEvent) && visit.at(&predecessor) ).implies( 
          position.at(&predecessor) < position.at(startEvent) 
        )
      );
//std::cerr << constraint.stringify() << std::endl;        
    }
  }

  std::vector<CP::Expression> visitedPositions;
  for ( Vertex& predecessor : parentExit.predecessors ) {
//std::cerr << predecessor.reference() << std::endl;        
    if ( 
      predecessor.type == Vertex::Type::EXIT &&
      ( predecessor.node->as<BPMN::FlowNode>()->parent == parentExit.node ) &&
      ( predecessor.node->represents<BPMN::Activity>() || 
        predecessor.node->represents<BPMN::MessageCatchEvent>() 
      )
    ) {
      visitedPositions.push_back( visit.at(&predecessor) * position.at(&predecessor) );        
    }
  }
  // ASSUMPTION: triggering of event-subprocesses does not depend on timer or conditional events in the main scope
  if ( visitedPositions.empty() ) {
    throw std::runtime_error("CPModel: event-subprocesses in a scope without activities and message catch events are not supported");
  }
  // if the event-subprocess is triggered, then the position must be before the last visited node that holds the token
//auto& constraint =
  model.addConstraint(
    visit.at(startEvent) * position.at(startEvent) <= CP::max( visitedPositions )
  );
//std::cerr << constraint.stringify() << std::endl;        
}


void CPModel::createRestrictions(const Vertex* vertex) {
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

CP::Expression CPModel::createExpression(const Vertex* vertex, const Model::Expression& expression) {
  auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  if ( !extensionElements ) {
    assert( vertex->node->represents<BPMN::FlowNode>()->parent );
    extensionElements = vertex->node->as<BPMN::FlowNode>()->parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  }

  auto compiled = LIMEX::Expression<CP::Expression,CP::Expression>(expression.expression, limexHandle);
  
  std::vector<CP::Expression> variables;
  for ( auto& variableName : compiled.getVariables() ) {
    auto attribute = extensionElements->attributeRegistry[variableName];
    if( attribute->type == ValueType::COLLECTION ) {
      throw std::runtime_error("CPModel: illegal expression '" + expression.expression + "'");
    }

    auto [defined,value] = getAttributeVariables(vertex,attribute);
//std::cerr << value.stringify() << std::endl;
    variables.push_back( value );
  }
  
  std::vector<CP::Expression> collectionVariables;
  for ( auto& variableName : compiled.getCollections() ) {
    auto attribute = extensionElements->attributeRegistry[variableName];
    if( attribute->type != ValueType::COLLECTION ) {
      throw std::runtime_error("CPModel: illegal expression '" + expression.expression + "'");
    }

    auto [defined,value] = getAttributeVariables(vertex,attribute);
//std::cerr << value.stringify() << std::endl;
    collectionVariables.push_back( value );
  }

  return compiled.evaluate(variables,collectionVariables);
}

void CPModel::createGlobalIndexVariable(const Vertex* vertex) {
  CP::Expression index;
  for ( auto& [modifierEntry, modifierExit] : flattenedGraph.globalModifiers ) {
    // create auxiliary variables indicating modifiers preceding the vertex
    index = index + model.addVariable(CP::Variable::Type::BOOLEAN, "weakly_precedes_{" + modifierExit.reference() + " → " + vertex->reference() + "}", position.at(&modifierExit) <= position.at(vertex) );
  }  
  globalIndex.emplace( vertex, model.addVariable(CP::Variable::Type::INTEGER, "globals_index_{" + vertex->reference() + "}", index ) );
}


void CPModel::createDataIndexVariables(const Vertex* vertex) {
  CP::reference_vector< const CP::Variable > dataIndices;
  dataIndices.reserve( vertex->dataOwners.size() );
  for ( Vertex& dataOwner : vertex->dataOwners ) {
    assert( flattenedGraph.dataModifiers.contains(&dataOwner) );   
    CP::Expression index;
    for ( auto& [modifierEntry, modifierExit] : flattenedGraph.dataModifiers.at(&dataOwner) ) {
      // create auxiliary variables indicating modifiers preceding the vertex
      index = index + model.addVariable(CP::Variable::Type::BOOLEAN, "weakly_precedes_{" + modifierExit.reference() + " → " + vertex->reference() + "}", position.at(&modifierExit) <= position.at(vertex) );
    }  
    // data index for data owner represents the number of modifiers exited according to the sequence positions
    dataIndices.emplace_back( model.addVariable(CP::Variable::Type::INTEGER, "data_index[" + dataOwner.node->id + "]_{" + vertex->reference() + "}", index ) );
  }
  dataIndex.emplace( vertex, std::move(dataIndices) );
}

void CPModel::addToObjective(const BPMNOS::Model::Attribute* attribute, const CP::Variable& variable) {
  if ( attribute->weight != 0.0 ) {
    model.setObjective( model.getObjective() + attribute->weight * variable );
  }
}

void CPModel::addObjectiveCoefficients(const Vertex* vertex) {
  assert( vertex->type == Vertex::Type::EXIT );
  auto loopCharacteristics = getLoopCharacteristics(vertex);
  if ( 
    flattenedGraph.dummies.contains(vertex) &&
    loopCharacteristics.has_value() &&
    loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
  ) {
    // for multi-instance activities, attributes are only added to vertices representing and instantiation 
    return;
  }

  if ( auto extensionElements = vertex->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
    for ( size_t i = extensionElements->attributeRegistry.statusAttributes.size() - extensionElements->attributes.size(); i < extensionElements->attributeRegistry.statusAttributes.size(); i++) {
      auto attribute = extensionElements->attributeRegistry.statusAttributes[i]; 
      addToObjective( attribute, status.at(vertex)[attribute->index].value );  
    }
  }
}

