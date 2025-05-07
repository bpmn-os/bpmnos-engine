#include "FlattenedGraph.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/StateMachine.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/CollectionRegistry.h"
#include <ranges>
#include <iostream>

using namespace BPMNOS::Execution;

/**********************
** Vertex
**********************/

FlattenedGraph::Vertex::Vertex(FlattenedGraph* graph, BPMNOS::number rootId, BPMNOS::number instanceId,  std::vector< size_t > loopIndices, const BPMN::Node* node, Type type, std::optional< std::pair<Vertex*, Vertex*> > parent)
  : graph(graph)
  , index(graph->vertices.size())
  , rootId(rootId)
  , instanceId(instanceId)
  , loopIndices(std::move(loopIndices))
  , node(node)
  , type(type)
  , parent(parent)
{
assert(node);
  if ( parent.has_value() ) {
    dataOwners = parent.value().first->dataOwners;
  }
}

const FlattenedGraph::Vertex* FlattenedGraph::Vertex::performer() const {
  assert( node->represents<BPMN::Activity>() );
  assert( node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );
/*
  auto performer = node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()->performer;
  const Vertex* entry = (type == Type::ENTRY ? this : this - 1);
  const Vertex* exit = (type == Type::EXIT ? this : this + 1);
  do {
    assert( entry->parent.has_value() );
    auto parentVertices = entry->parent.value();
    entry = &parentVertices.first;
    exit = &parentVertices.second;
  } while ( entry->node != performer );

  return entry;
*/
  auto adHocSubProcess = node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
  assert( adHocSubProcess );
  assert( parent.has_value() );
  
  auto vertex = parent.value().first;
  while (vertex->node && vertex->node != adHocSubProcess->performer) {
    assert( vertex->parent.has_value() );
    vertex = vertex->parent.value().first;
  }
  return vertex;
}

size_t FlattenedGraph::Vertex::dataOwnerIndex( const BPMNOS::Model::Attribute* attribute ) const {
  assert( attribute->category == BPMNOS::Model::Attribute::Category::DATA );
  for ( size_t index = 0; index < dataOwners.size(); index++) {
    auto extensionElements = dataOwners[index]->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( 
      attribute->index < extensionElements->attributeRegistry.dataAttributes.size() &&
      attribute->index >= extensionElements->attributeRegistry.dataAttributes.size() - extensionElements->data.size()
    ) {
      return index;
    }
  }
  throw std::logic_error("FlattenedGraph: unable to determine data owner index for '" + attribute->id + "'");
}

std::pair< const FlattenedGraph::Vertex*, const FlattenedGraph::Vertex*> FlattenedGraph::Vertex::dataOwner( const BPMNOS::Model::Attribute* attribute ) const {
  auto index = dataOwnerIndex(attribute);
  return { dataOwners[index], (dataOwners[index] + 1) };
}

std::string FlattenedGraph::Vertex::reference() const {
  return shortReference() + "," + ( type == Type::ENTRY ? "entry" : "exit" );
}

std::string FlattenedGraph::Vertex::shortReference() const {
  std::string result = BPMNOS::to_string(instanceId, STRING) + "," + node->id;
  for ( auto index : loopIndices ) {
    result += "°" + std::to_string(index);
  }
  return result;
}

nlohmann::ordered_json FlattenedGraph::Vertex::jsonify() const {
  nlohmann::ordered_json jsonObject;
  jsonObject["vertex"] = reference();
  jsonObject["inflows"] = nlohmann::json::array();
  for ( auto& [_,vertex] : inflows ) {
    jsonObject["inflows"].push_back(vertex->reference());
  }
  jsonObject["outflows"] = nlohmann::json::array();
  for ( auto& [_,vertex] : outflows ) {
    jsonObject["outflows"].push_back(vertex->reference());
  }
  jsonObject["predecessors"] = nlohmann::json::array();
  for ( auto vertex : predecessors ) {
    jsonObject["predecessors"].push_back(vertex->reference());
  }
  jsonObject["successors"] = nlohmann::json::array();
  for ( auto vertex : successors ) {
    jsonObject["successors"].push_back(vertex->reference());
  }

  return jsonObject;
}

/**********************
** FlattenedGraph
**********************/

FlattenedGraph::FlattenedGraph(const BPMNOS::Model::Scenario* scenario) : scenario(*scenario) {
  // get all known instances
  auto instances = this->scenario.getKnownInstances( this->scenario.getInception() );
//std::cerr << "Instances: " << instances.size() << std::endl;
  for ( auto& instance : instances ) {
    addInstance( instance );
  }
}

nlohmann::ordered_json FlattenedGraph::jsonify() const {
  nlohmann::ordered_json jsonObject = nlohmann::json::array();
  for ( auto& vertex : vertices ) {
    jsonObject.push_back(vertex->jsonify());
  }
  return jsonObject;
}

void FlattenedGraph::addInstance( const BPMNOS::Model::Scenario::InstanceData* instance ) {
//std::cerr << "Add instance: " << instance->id << std::endl;
  // create process vertices
  auto [ entry, exit ] = createVertexPair(instance->id, instance->id, {}, instance->process, std::nullopt);
  initialVertices.push_back(entry);
  flatten( instance->id, instance->process, entry, exit );
}

void FlattenedGraph::addNonInterruptingEventSubProcess( const BPMN::EventSubProcess* eventSubProcess, Vertex* parentEntry, Vertex* parentExit ) {
  nonInterruptingEventSubProcesses.emplace_back(eventSubProcess, parentEntry, parentExit, 0, nullptr);
  // iterate through all known trigger and flatten event-subprocess instantiations
  auto counter = std::get<3>( nonInterruptingEventSubProcesses.back() );
  auto lastStart = std::get<4>( nonInterruptingEventSubProcesses.back() );
  assert( eventSubProcess->startEvent->represents<BPMN::MessageStartEvent>() );
  assert( eventSubProcess->startEvent->extensionElements );
  assert( eventSubProcess->startEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = eventSubProcess->startEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto candidate : candidates ) {
    if( sendingVertices.contains(candidate) ) {
      for ( [[maybe_unused]] auto& _ : sendingVertices.at(candidate) ) {
        // create and flatten next event-subprocess
        counter++;
        BPMNOS::number id = 
          BPMNOS::to_number(
            BPMNOS::to_string(parentEntry->instanceId,STRING) + 
            BPMNOS::Model::Scenario::delimiters[0] + 
            eventSubProcess->id + 
            BPMNOS::Model::Scenario::delimiters[1] + 
            std::to_string(counter)
          , STRING)
        ;
        flatten( id, eventSubProcess, parentEntry, parentExit );
        // newly created vertices at start event must succeed previous vertices at start event
        auto& [startEntry,startExit] = vertexMap.at({ id, parentEntry->loopIndices, eventSubProcess->startEvent });
        if ( lastStart ) {
          lastStart->successors.push_back(startEntry);
          startEntry->predecessors.push_back(lastStart);
        }
        lastStart = startExit;
      }
    }
  }
}

void FlattenedGraph::addSender( const BPMN::MessageThrowEvent* messageThrowEvent, Vertex* senderEntry, Vertex* senderExit ) {
//std::cerr << "Add sender: " << senderEntry.reference() << std::endl;
  // flatten event subprocesses if applicable
  assert( messageThrowEvent->extensionElements );
  assert( messageThrowEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageThrowEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto& [eventSubProcess, parentEntry, parentExit, counter, lastStart] : nonInterruptingEventSubProcesses ) {
    if (std::find(candidates.begin(), candidates.end(), eventSubProcess->startEvent) != candidates.end()) {
      // TODO: check header
      // eventSubProcess may be triggered by message throw event, create and flatten next event-subprocess
      counter++;
//std::cerr << "Add non-interrupting event-subprocess: " << eventSubProcess->id << "#" << counter << std::endl;
      BPMNOS::number id = BPMNOS::to_number( BPMNOS::to_string(parentEntry->instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + eventSubProcess->id + BPMNOS::Model::Scenario::delimiters[1] + std::to_string(counter),  STRING);
      flatten( id, eventSubProcess, parentEntry, parentExit );
      // newly created vertices at start event must succeed previous vertices at start event
      auto& [startEntry,startExit] = vertexMap.at({ id, parentEntry->loopIndices, eventSubProcess->startEvent });
      if ( lastStart ) {
        lastStart->successors.push_back(startEntry);
        startEntry->predecessors.push_back(lastStart);
      }
      lastStart = startExit;
    }
  }
  
  // set precedences for all recipients
  for ( auto candidate : candidates ) {
    for ( auto& [ recipientEntry, recipientExit] : receivingVertices[candidate] ) {
      // TODO: check header
      senderEntry->recipients.push_back(recipientExit);
      recipientExit->senders.push_back(senderEntry);
      if ( messageThrowEvent->represents<BPMN::SendTask>() ) {
        recipientExit->recipients.push_back(senderExit);
        senderExit->senders.push_back(recipientExit);
      }
    }
  }
  
  sendingVertices[messageThrowEvent].emplace_back(senderEntry, senderExit);
}

void FlattenedGraph::addRecipient( const BPMN::MessageCatchEvent* messageCatchEvent, Vertex* recipientEntry, Vertex* recipientExit ) {
  // set precedences for all senders
  assert( messageCatchEvent->extensionElements );
  assert( messageCatchEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageCatchEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto candidate : candidates ) {
    for ( auto& [ senderEntry, senderExit] : sendingVertices[candidate] ) {
      // TODO: check header
      senderEntry->recipients.push_back(recipientExit);
      recipientExit->senders.push_back(senderEntry);
      if ( candidate->represents<BPMN::SendTask>() ) {
        recipientExit->recipients.push_back(senderExit);
        senderExit->senders.push_back(recipientExit);
      }
    }
  }

  receivingVertices[messageCatchEvent].emplace_back(recipientEntry, recipientExit);
}

const BPMNOS::Model::Attribute* FlattenedGraph::getLoopIndexAttribute(const BPMN::Activity* activity) const {
  auto extensionElements = activity->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);
  const BPMNOS::Model::Attribute* attribute = 
    extensionElements->loopIndex.has_value() ? 
    extensionElements->loopIndex.value()->expression->isAttribute() :
    nullptr
  ;
  if ( !attribute ) {
    throw std::runtime_error("FlattenedGraph: unable to determine loop index for activity '" + activity->id + "'");
  }
  assert( attribute->category == BPMNOS::Model::Attribute::Category::STATUS );
  return attribute;
} 

std::pair<FlattenedGraph::Vertex*, FlattenedGraph::Vertex*> FlattenedGraph::createVertexPair(BPMNOS::number rootId, BPMNOS::number instanceId, std::vector< size_t > loopIndices, const BPMN::Node* node, std::optional< std::pair<Vertex*, Vertex*> > parent) {
  vertices.emplace_back(std::make_unique<Vertex>(this, rootId, instanceId, loopIndices, node, Vertex::Type::ENTRY, parent));
  auto entry = vertices.back().get();
  vertices.emplace_back(std::make_unique<Vertex>(this, rootId, instanceId, loopIndices, node, Vertex::Type::EXIT, parent));
  auto exit = vertices.back().get();
//std::cerr << "Add vertex pair: " << entry.reference() << std::endl;
  if ( node->represents<BPMN::Scope>() ) {
    entry->successors.push_back(exit);
    exit->predecessors.push_back(entry);
  }
  else {
    entry->outflows.emplace_back(nullptr,exit);
    exit->inflows.emplace_back(nullptr,entry);
  }
  
  if ( parent.has_value() ) {
    entry->predecessors.push_back( parent.value().first );
    exit->successors.push_back( parent.value().second ); 
    parent.value().first->successors.push_back( entry ); 
    parent.value().second->predecessors.push_back( exit );
  }
  
  vertexMap.emplace( {instanceId,loopIndices,node}, {entry,exit} );

  if ( !loopIndexAttributes.contains(node) ) {
    std::vector< const BPMNOS::Model::Attribute* > attributes = (
      parent.has_value() ? 
      loopIndexAttributes.at(parent.value().first->node) :
      std::vector< const BPMNOS::Model::Attribute* >()
    );

    if ( auto activity = node->represents<BPMN::Activity>();
      activity &&
      activity->loopCharacteristics.has_value() &&
      activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard
    ) {
      attributes.push_back( getLoopIndexAttribute(activity) );
    } 
//std::cerr << node->id << " has " << attributes.size() << " loop index attributes" << std::endl;
    loopIndexAttributes[node] = std::move(attributes);
  }
  
  assert( node->extensionElements );
  auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();

  if ( !extensionElements ) {
    return { entry, exit };
  }
  
  if ( auto messageThrowEvent = node->represents<BPMN::MessageThrowEvent>() ) {
    addSender( messageThrowEvent, entry, exit );
  }
  else if ( auto messageCatchEvent = node->represents<BPMN::MessageCatchEvent>() ) {
    addRecipient( messageCatchEvent, entry, exit );
  }

  if ( extensionElements->data.size() ) {
    entry->dataOwners.push_back( entry );
    exit->dataOwners.push_back( entry );
  }
  
  // populate lookup maps of sequential activities for each performer
  if ( extensionElements->hasSequentialPerformer ) {
    sequentialActivities.emplace(entry,std::vector< std::pair<const Vertex*, const Vertex*> >());
  }
  else if ( auto sequentialAdHocSubProcess = node->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
    sequentialAdHocSubProcess && !sequentialAdHocSubProcess->performer
  ) {
    sequentialActivities.emplace(entry,std::vector< std::pair<const Vertex*, const Vertex*> >());
  }

  if ( auto activity = node->represents<BPMN::Activity>();
    activity && 
    extensionElements->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
    sequentialActivities.at( entry->performer() ).push_back( { entry, exit } );
  }

  // populate lookup maps of data modifiers for data owner
  if ( extensionElements->data.size() ) {
    dataModifiers.emplace(entry,std::vector< std::pair<const Vertex*, const Vertex*> >());
  }

  if ( node->represents<BPMN::Task>() ) {
    std::unordered_set< const Vertex* > dataOwners;
    bool modifiesGlobals = false;
    for ( auto& operator_ : extensionElements->operators ) {
      if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        dataOwners.emplace( entry->dataOwner(operator_->attribute).first );
      }
      else if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL ) {
        modifiesGlobals = true;
      }
    }
    for ( auto dataOwner : dataOwners ) {
      assert( dataModifiers.contains( dataOwner ) );
      dataModifiers.at( dataOwner ).push_back( { entry, exit } );
    }
    if ( modifiesGlobals ) {
      globalModifiers.push_back( { entry, exit } );
    }
  }

  if ( auto typedStartEvent = node->represents<BPMN::TypedStartEvent>() ) {
    extensionElements = typedStartEvent->parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    std::unordered_set< const Vertex* > dataOwners;
    bool modifiesGlobals = false;
    for ( auto& operator_ : extensionElements->operators ) {
      if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        dataOwners.emplace( entry->dataOwner(operator_->attribute).first );
      }
      else if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL ) {
        modifiesGlobals = true;
      }
    }
    for ( auto dataOwner : dataOwners ) {
      assert( dataModifiers.contains( dataOwner ) );
      dataModifiers.at( dataOwner ).push_back( { entry, exit } );
    }
    if ( modifiesGlobals ) {
      globalModifiers.push_back( { entry, exit } );
    }
  }
  
  return { entry, exit };
}

void FlattenedGraph::createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, std::vector< size_t > loopIndices, const BPMN::Activity* activity, std::optional< std::pair<Vertex*, Vertex*> > parent) {
  // loop & multi-instance activties

  // create main vertices
  vertices.emplace_back(std::make_unique<Vertex>(this, rootId, instanceId, loopIndices, activity, Vertex::Type::ENTRY, parent));
  auto entry = vertices.back().get();
  vertices.emplace_back(std::make_unique<Vertex>(this, rootId, instanceId, loopIndices, activity, Vertex::Type::EXIT, parent));
  auto exit = vertices.back().get();
  entry->successors.push_back(exit);
  exit->predecessors.push_back(entry);

  vertexMap.emplace( {instanceId,loopIndices,activity}, {entry,exit} );
  dummies.insert( entry );
  dummies.insert( exit );
  assert( parent.has_value() );
    
  entry->predecessors.push_back( parent.value().first );
  exit->successors.push_back( parent.value().second ); 
  parent.value().first->successors.push_back( entry ); 
  parent.value().second->predecessors.push_back( exit );
  
  // create loopIndexAttributes
  if ( !loopIndexAttributes.contains(activity) ) { 
    assert( loopIndexAttributes.contains(parent.value().first->node) );
    auto attributes = loopIndexAttributes.at(parent.value().first->node);
    if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
      attributes.push_back( getLoopIndexAttribute(activity) );
    } 
//std::cerr << activity->id << " has " << attributes.size() << " loop index attributes" << std::endl;
    loopIndexAttributes[activity] = std::move(attributes);
  }
  
  // lambda returning parameter value known at time zero
  auto getValue = [&](BPMNOS::Model::Parameter* parameter) -> std::optional<BPMNOS::number> {
    if ( parameter->expression ) {
//std::cerr << parameter->expression->expression << std::endl;
      // collect variable values
      std::vector< double > variableValues;
      for ( auto attribute : parameter->expression->variables ) {
        if ( !attribute->isImmutable ) {
          throw std::runtime_error("FlattenedGraph: Loop parameter '" + parameter->name + "' for activity '" + activity->id +"' must be immutable" );
        }
        auto value = scenario.getKnownValue(rootId, attribute, scenario.getInception() );
        if ( !value.has_value() ) {
//std::cerr << "unknown value of '" << attribute->id << "'" << std::endl;
          // return nullopt because required attribute value is not given
          return std::nullopt;        
        }
        variableValues.push_back( (double)value.value() );
      }

      // collect values of all variables in collection
      std::vector< std::vector< double > > collectionValues;
      for ( auto attribute : parameter->expression->collections ) {
        if ( !attribute->isImmutable ) {
          throw std::runtime_error("FlattenedGraph: Loop parameter '" + parameter->name + "' for activity '" + activity->id +"' must be immutable" );
        }
        collectionValues.push_back( {} );
        auto collection = scenario.getKnownValue(rootId, attribute, scenario.getInception() );
        if ( !collection.has_value() ) {
//std::cerr << "unknown collection value of '" << attribute->id << "'" << std::endl;
          // return nullopt because required collection is not given
          return std::nullopt;
        }
        for ( auto value : collectionRegistry[(size_t)collection.value()] ) {
          collectionValues.back().push_back( value );
        }
      }

//std::cerr << parameter->expression->expression << " = " << number(parameter->expression->compiled.evaluate(variableValues,collectionValues)) << std::endl;
      return number(parameter->expression->compiled.evaluate(variableValues,collectionValues));      
    }

    return std::nullopt;
  };
  
  // determine number of vertices to be created
  int n = 0;
  auto extensionElements = activity->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
/*
    if( activity->represents<BPMN::SubProcess>() ) {
      throw std::runtime_error("FlattenedGraph: loop subprocesses are not yet supported" );
    }
*/
    if ( extensionElements->loopMaximum.has_value() ) {
      auto value = getValue( extensionElements->loopMaximum.value().get() ); 
      n = value.has_value() ? (int)value.value() : 0;
    }
  }
  else {
    if ( extensionElements->loopCardinality.has_value() ) {
      auto value = getValue( extensionElements->loopCardinality.value().get() );
      n = value.has_value() ? (int)value.value() : 0;
    }
  }

  if ( n <= 0 ) {
    throw std::runtime_error("FlattenedGraph: cannot determine loop maximum/cardinality for activity '" + activity->id +"'" );
  }



//  auto& container = vertexMap[activity][instanceId]; // get or create container
//  container.emplace_back( entry );
 
  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
//std::cerr << "Standard" << std::endl;
    // create vertices for loop activity
    Vertex* previous = entry;
    loopIndices.push_back(0);
    assert( loopIndexAttributes.contains(previous->node) );
//std::cerr << entry.reference() << ": " << loopIndices.size() << "/" << loopIndexAttributes.at(entry.node).size() << std::endl; 
    assert( loopIndices.size() == loopIndexAttributes.at(previous->node).size() );
    for ( size_t i = 1; i <= (size_t)n; i++ ) {
      loopIndices.back() = i;
      auto [ loopingEntry, loopingExit ] = createVertexPair(rootId, instanceId, loopIndices, activity, parent);
      // every looping entry vertex has an inflow from the previous
      loopingEntry->inflows.emplace_back(nullptr,previous);
      previous->outflows.emplace_back(nullptr,loopingEntry);
      
      // every looping exit can be the last
      exit->inflows.emplace_back(nullptr,loopingExit );
      loopingExit->outflows.emplace_back(nullptr,exit );

      previous = loopingExit;

      if( auto subprocess = activity->represents<BPMN::SubProcess>() ) {
        flatten(instanceId, subprocess, loopingEntry, loopingExit );
      }
    }
  }
  else {
//std::cerr << "MultiInstance" << std::endl;
   std::string baseName = BPMNOS::to_string(instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + activity->id + BPMNOS::Model::Scenario::delimiters[1] ;
    // create vertices for multi-instance activity
    Vertex* previous = nullptr;
    for ( int i = 1; i <= n; i++ ) {
      auto multiInstanceId = BPMNOS::to_number(baseName + std::to_string(i),STRING);
      auto [ multiInstanceEntry, multiInstanceExit ] = createVertexPair(rootId, multiInstanceId, loopIndices, activity, parent);
      // every multi-instance entry vertex has an inflow from the main entry
      multiInstanceEntry->inflows.emplace_back(nullptr,entry);
      entry->outflows.emplace_back(nullptr,multiInstanceEntry);
      // every multi-instance exit vertex has an outflow to the main exit
      multiInstanceExit->outflows.emplace_back(nullptr,exit);
      exit->inflows.emplace_back(nullptr,multiInstanceExit);

      if (
        previous &&
        activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential
      ) {
        previous->successors.push_back(multiInstanceEntry);
        multiInstanceEntry->predecessors.push_back(previous);
      }
      previous = multiInstanceExit;

      if( auto subprocess = activity->represents<BPMN::SubProcess>() ) {
        flatten(multiInstanceId, subprocess, multiInstanceEntry, multiInstanceExit );
      }
    }
  }

//std::cerr << "done" << std::endl;

}

void FlattenedGraph::flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex* scopeEntry, Vertex* scopeExit) {
//std::cerr << "flatten: " << BPMNOS::to_string(instanceId,STRING) << "/" << scopeEntry.shortReference() << std::endl;
  std::pair<Vertex*, Vertex*> parent = {scopeEntry,scopeExit};
  
  if ( scope->flowNodes.empty() ) {
    scopeEntry->outflows.emplace_back(nullptr,scopeExit);
    scopeExit->inflows.emplace_back(nullptr,scopeEntry);
    return;
//    throw std::runtime_error("FlattenedGraph: unable to flatten empty scope '" + scope->id + "'");
  }
  for ( auto& flowNode : scope->flowNodes ) {
    
    // create vertices for flow node
    if ( auto activity = flowNode->represents<BPMN::Activity>();
      activity && 
      activity->loopCharacteristics.has_value() 
    ) {
      // create copies for loop and multi-instance activities
      createLoopVertices(scopeEntry->rootId, instanceId, scopeEntry->loopIndices, activity, parent);
    }
    else {
      createVertexPair(scopeEntry->rootId, instanceId, scopeEntry->loopIndices, flowNode, parent);
    }

    auto& [flowNodeEntry,flowNodeExit] = vertexMap.at({instanceId,scopeEntry->loopIndices,flowNode});

    if ( flowNode->represents<BPMN::UntypedStartEvent>() || flowNode->represents<BPMN::TypedStartEvent>() ) {
      flowNodeEntry->inflows.emplace_back(nullptr,scopeEntry);
      scopeEntry->outflows.emplace_back(nullptr,flowNodeEntry);
    }

    if ( flowNode->represents<BPMN::Activity>() && flowNode->incoming.empty() ) {
      if( !flowNode->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        throw std::runtime_error("FlattenedGraph: only activities within adhoc subprocesses maybe without incoming sequence flow");
      }
      flowNodeEntry->inflows.emplace_back(nullptr,scopeEntry);
      scopeEntry->outflows.emplace_back(nullptr,flowNodeEntry);
    }
    
    if ( flowNode->outgoing.empty() ) {
      // flow node without outgoing sequence flow
      scopeExit->inflows.emplace_back(nullptr,flowNodeExit);
      flowNodeExit->outflows.emplace_back(nullptr,scopeExit);
    }

    // flatten child scopes
    if ( auto subprocess = flowNode->represents<BPMN::SubProcess>();
      subprocess && 
      subprocess->loopCharacteristics.has_value() 
    ) {
      // nothing to be done because loops of each subprocess are already flattened
    }
    else if ( auto childScope = flowNode->represents<BPMN::Scope>() ) {
      flatten( flowNodeEntry->instanceId, childScope, flowNodeEntry, flowNodeExit );
    }

  }

//std::cerr << "sequence flows: " << scope->sequenceFlows.size() << std::endl;
  // sequence flows
  for ( auto& sequenceFlow : scope->sequenceFlows ) {
    // create unique flow from origin to destination
    Vertex* origin = vertexMap.at({instanceId,scopeEntry->loopIndices,sequenceFlow->source}).second;
    Vertex* destination = vertexMap.at({instanceId,scopeEntry->loopIndices,sequenceFlow->target}).first;
    origin->outflows.emplace_back(sequenceFlow.get(),destination);
    destination->inflows.emplace_back(sequenceFlow.get(),origin);
  }

  // boundary events
  for ( auto& flowNode : scope->flowNodes ) {
    if ( auto boundaryEvent = flowNode->represents<BPMN::BoundaryEvent>() ) {
      throw std::runtime_error("FlattenedGraph: Boundary event '" + boundaryEvent->id + "' is not supported");
    }    
  }
    
  // event-subprocesses
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    if ( eventSubProcess->startEvent->isInterrupting ) {
      // interrupting event-subprocesses
      throw std::runtime_error("FlattenedGraph: Interrupting event-subprocess '" + eventSubProcess->id + "' is not supported");
//      flatten( instanceId, eventSubProcess, scopeEntry, scopeExit );
    }
    else {
      // non-interrupting event-subprocesses
      if ( eventSubProcess->startEvent->represents<BPMN::MessageStartEvent>() ) {
        addNonInterruptingEventSubProcess(eventSubProcess, scopeEntry, scopeExit);
      }
      else {
        throw std::runtime_error("FlattenedGraph: Type of non-interrupting event-subprocess '" + eventSubProcess->id + "' is not supported");
      }
    }
  }
  
  // compensation nodes
  for ( auto& flowNode : scope->flowNodes ) {
    auto activity = flowNode->represents<BPMN::Activity>();
    if ( activity && activity->isForCompensation ) {
      throw std::runtime_error("FlattenedGraph: Compensation activity '" + activity->id + "' is not supported");
    }
  } 
}

bool FlattenedGraph::modifiesData(const Vertex* vertex, const Vertex* dataOwner) const {
  for ( auto& [entry,exit] : dataModifiers.at( dataOwner ) ) {
    if ( vertex == entry || vertex == exit ) return true; 
  }
  return false;
}

bool FlattenedGraph::modifiesGlobals(const Vertex* vertex) const {
  for ( auto& [entry,exit] : globalModifiers ) {
    if ( vertex == entry || vertex == exit ) return true; 
  }
  return false;
}

std::vector< const FlattenedGraph::Vertex* > FlattenedGraph::getSortedVertices() const {
  std::vector< const Vertex* > sortedVertices;
  sortedVertices.reserve( vertices.size() );
  for ( auto initialVertex : initialVertices ) {
    std::unordered_map<const Vertex*, size_t> inDegree;
  
    std::deque<const Vertex*> queue;
    queue.push_back(initialVertex);
    
    // determine vertices in topological order
    while ( !queue.empty() ) {
//std::cerr << "Queue " << queue.size() << std::endl;
      const Vertex* current = queue.front();
      queue.pop_front();
      sortedVertices.push_back(current);
      inDegree.erase(current);
    
      for ( auto& [_,vertex] : current->outflows ) {
        if ( !inDegree.contains(vertex) ) {
          // initialize in degree
          inDegree[vertex] = vertex->inflows.size() + vertex->predecessors.size();
        }
        // decrement in degree and add vertex to queue if zero
        if ( --inDegree.at(vertex) == 0 ) {
          queue.push_back(vertex);
        }
      }
      for ( auto vertex : current->successors ) {
        if ( !inDegree.contains(vertex) ) {
          // initialize in degree
          inDegree[vertex] = vertex->inflows.size() + vertex->predecessors.size();
        }
//std::cerr << "Vertex " << vertex.reference() << " has inDegree " << inDegree.at(&vertex) << "/" << vertex.inflows.size() << "/" << vertex.predecessors.size() << std::endl;
      // decrement in degree and add vertex to queue if zero
        if ( --inDegree.at(vertex) == 0 ) {
          queue.push_back(vertex);
        }
      }
    }
  
    if ( inDegree.size() ) {
      throw std::runtime_error("FlattenedGraph: cycle detected in '" + BPMNOS::to_string(initialVertex->rootId, STRING) + "'");
    }
  }
//std::cerr << "sortedVertices:" << sortedVertices.size() << std::endl;
  return sortedVertices;
}

const FlattenedGraph::Vertex* FlattenedGraph::getVertex( const Token* token ) const {
//std::cerr << "getVertex(" << token->jsonify() << ")" << std::endl;
  auto node = token->node ? token->node->as<BPMN::Node>() : token->owner->process->as<BPMN::Node>();
  if( !loopIndexAttributes.contains(node) ) {
    // unreachable typed start event
    assert( node->represents<BPMN::TypedStartEvent>() );
    return nullptr;    
  }
  
  // determine loop indices from token
  std::vector< size_t > loopIndices;
  for ( auto attribute : loopIndexAttributes.at(node)  ) {
//std::cerr << attribute->id << std::endl;
    if ( !token->status.at(attribute->index).has_value() ) {
      // loop index has not yet been set
      break;
    }
    loopIndices.push_back( (size_t)token->status.at(attribute->index).value() );
  }
  
  auto instanceId = token->data->at(BPMNOS::Model::ExtensionElements::Index::Instance).get().value();
//std::cerr << stringRegistry[(size_t)instanceId] << "/" << node->id << "/" << token->jsonify() << std::endl;
  if( !vertexMap.contains({instanceId,loopIndices,node}) ) {
    return nullptr;
  }
  assert( vertexMap.contains({instanceId,loopIndices,node}) );
  auto& [entry,exit] = vertexMap.at({instanceId,loopIndices,node});
  return (token->state == Token::State::ENTERED) ? entry : exit;
}

const FlattenedGraph::Vertex* FlattenedGraph::entry(const Vertex* vertex) const {
  return vertices[ vertex->index - ( vertex->type == Vertex::Type::EXIT ) ].get();
}

const FlattenedGraph::Vertex* FlattenedGraph::exit(const Vertex* vertex) const {
  return vertices[ vertex->index + ( vertex->type == Vertex::Type::ENTRY ) ].get();
}

