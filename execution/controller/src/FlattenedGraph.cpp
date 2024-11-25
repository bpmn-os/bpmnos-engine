#include "FlattenedGraph.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/CollectionRegistry.h"
#include <ranges>
#include <iostream>

using namespace BPMNOS::Execution;

FlattenedGraph::Vertex::Vertex(size_t index, BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node, Type type)
  : index(index)
  , rootId(rootId)
  , instanceId(instanceId)
  , node(node)
  , type(type)
{
  if ( node->represents<BPMN::ChildNode>() ) {
    dataOwners = parent().first.dataOwners;
  }
}


std::pair<const FlattenedGraph::Vertex&, const FlattenedGraph::Vertex&>  FlattenedGraph::Vertex::parent() const {
  if( !node->represents<BPMN::ChildNode>() ) {
    throw std::logic_error("FlattenedGraph::Vertex: only child nodes have a parent");
  }

  assert( predecessors.size() );
  assert( predecessors.back().get().node->represents<BPMN::Scope>() );
  assert( predecessors.back().get().node == node->as<BPMN::ChildNode>()->parent );

  assert( successors.size() );
  assert( successors.back().get().node->represents<BPMN::Scope>() );
  assert( successors.back().get().node == node->as<BPMN::ChildNode>()->parent );

  return std::pair<Vertex&, Vertex&>(predecessors.back().get(), successors.back().get() );
}

std::pair<const FlattenedGraph::Vertex&, const FlattenedGraph::Vertex&> FlattenedGraph::Vertex::performer() const {
  assert( node->represents<BPMN::Activity>() );
  assert( node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() );

  auto performer = node->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()->performer;
  const Vertex* entry = (type == Type::ENTRY ? this : this - 1);
  const Vertex* exit = (type == Type::EXIT ? this : this + 1);
  do {
    auto parentVertices = entry->parent();
    entry = &parentVertices.first;
    exit = &parentVertices.second;
  } while ( entry->node != performer );

  return { *entry, *exit };
}

std::pair< const FlattenedGraph::Vertex&, const FlattenedGraph::Vertex&> FlattenedGraph::Vertex::dataOwner( const BPMNOS::Model::Attribute* attribute ) const {
  assert( attribute->category == BPMNOS::Model::Attribute::Category::DATA );
  const Vertex* entry = (type == Type::ENTRY ? this : this - 1);
  const Vertex* exit = (type == Type::EXIT ? this : this + 1);
  const BPMNOS::Model::ExtensionElements* extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  while ( extensionElements->attributeRegistry.dataAttributes.size() - extensionElements->data.size() > attribute->index ) {
    auto parentVertices = entry->parent();
    entry = &parentVertices.first;
    exit = &parentVertices.second;
    extensionElements = entry->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  }

  return { *entry, *exit };
}

std::string FlattenedGraph::Vertex::reference() const {
  return BPMNOS::to_string(instanceId, STRING) + "," + node->id + "," + ( type == Type::ENTRY ? "entry" : "exit" );
}


FlattenedGraph::FlattenedGraph(const BPMNOS::Model::Scenario* scenario) : scenario(scenario) {
  // get all known instances
  auto instances = scenario->getKnownInstances(0);
  for ( auto& instance : instances ) {
    addInstance( instance );
  }
}

void FlattenedGraph::addInstance( const BPMNOS::Model::Scenario::InstanceData* instance ) {
  // create process vertices
  auto [ entry, exit ] = createVertexPair(instance->id, instance->id, instance->process);
  initialVertices.push_back(entry);
  flatten( instance->id, instance->process, entry, exit );
}

void FlattenedGraph::addNonInterruptingEventSubProcess( const BPMN::EventSubProcess* eventSubProcess, Vertex& parentEntry, Vertex& parentExit ) {
  nonInterruptingEventSubProcesses.emplace_back(eventSubProcess, parentEntry, parentExit, 0, nullptr);
  // iterate through all known trigger and flatten event-subprocess instantiations
  auto& counter = std::get<unsigned int>( nonInterruptingEventSubProcesses.back() );
  auto& lastStart = std::get<Vertex*>( nonInterruptingEventSubProcesses.back() );
  assert( eventSubProcess->startEvent->represents<BPMN::MessageStartEvent>() );
  assert( eventSubProcess->startEvent->extensionElements );
  assert( eventSubProcess->startEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = eventSubProcess->startEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto candidate : candidates ) {
    for ( [[maybe_unused]] auto& _ : sendingVertices[candidate] ) {
      // create and flatten next event-subprocess
      counter++;
      BPMNOS::number id = BPMNOS::to_number( BPMNOS::to_string(parentEntry.instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + eventSubProcess->id + BPMNOS::Model::Scenario::delimiters[1] + std::to_string(counter),  STRING);
      flatten( id, eventSubProcess, parentEntry, parentExit );
      // newly created vertices at start event must succeed previous vertices at start event
      auto& container = vertexMap.at(eventSubProcess->startEvent).at(id);
      assert( container.size() >= 2 );
      Vertex& startExit = container[container.size()-1];
      if ( lastStart ) {
        Vertex& startEntry = container[container.size()-2];
        lastStart->successors.push_back(startEntry);
        startEntry.predecessors.push_back(*lastStart);
      }
      lastStart = &startExit;
    }
  } 
}

void FlattenedGraph::addSender( const BPMN::MessageThrowEvent* messageThrowEvent, Vertex& senderEntry, Vertex& senderExit ) {
  // flatten event subprocesses if applicable
  assert( messageThrowEvent->extensionElements );
  assert( messageThrowEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageThrowEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto& [eventSubProcess, parentEntry, parentExit, counter, lastStart] : nonInterruptingEventSubProcesses ) {
    if (std::find(candidates.begin(), candidates.end(), eventSubProcess->startEvent) != candidates.end()) {
      // eventSubProcess may be triggered by message throw event, create and flatten next event-subprocess
      counter++;
      BPMNOS::number id = BPMNOS::to_number( BPMNOS::to_string(parentEntry.instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + eventSubProcess->id + BPMNOS::Model::Scenario::delimiters[1] + std::to_string(counter),  STRING);
      flatten( id, eventSubProcess, parentEntry, parentExit );
    }
  }
  
  // set precedences for all recipients
  for ( auto candidate : candidates ) {
    for ( auto& [ recipientEntry, recipientExit] : receivingVertices[candidate] ) {
      senderEntry.recipients.push_back(recipientExit);
      recipientExit.senders.push_back(senderEntry);
      if ( messageThrowEvent->represents<BPMN::SendTask>() ) {
        recipientExit.recipients.push_back(senderExit);
        senderExit.senders.push_back(recipientExit);
      }
    }
  }
  
  sendingVertices[messageThrowEvent].emplace_back(senderEntry, senderExit);
}

void FlattenedGraph::addRecipient( const BPMN::MessageCatchEvent* messageCatchEvent, Vertex& recipientEntry, Vertex& recipientExit ) {
  // set precedences for all senders
  assert( messageCatchEvent->extensionElements );
  assert( messageCatchEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageCatchEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto candidate : candidates ) {
    for ( auto& [ senderEntry, senderExit] : sendingVertices[candidate] ) {
      senderEntry.recipients.push_back(recipientExit);
      recipientExit.senders.push_back(senderEntry);
      if ( candidate->represents<BPMN::SendTask>() ) {
        recipientExit.recipients.push_back(senderExit);
        senderExit.senders.push_back(recipientExit);
      }
    }
  }

  receivingVertices[messageCatchEvent].emplace_back(recipientEntry, recipientExit);
}

std::pair<FlattenedGraph::Vertex&, FlattenedGraph::Vertex&> FlattenedGraph::createVertexPair(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node) {
  vertices.emplace_back(vertices.size(), rootId, instanceId, node, Vertex::Type::ENTRY);
  auto& entry = vertices.back();
  vertices.emplace_back(vertices.size(), rootId, instanceId, node, Vertex::Type::EXIT);
  auto& exit = vertices.back();

  entry.successors.push_back(exit);
  exit.predecessors.push_back(entry);
  
  auto& container = vertexMap[node][instanceId]; // get or create container
  container.emplace_back( entry );
  container.emplace_back( exit );
  
  if ( auto messageThrowEvent = node->represents<BPMN::MessageThrowEvent>() ) {
    addSender( messageThrowEvent, entry, exit );
  }
  else if ( auto messageCatchEvent = node->represents<BPMN::MessageCatchEvent>() ) {
    addRecipient( messageCatchEvent, entry, exit );
  }

  assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  if ( extensionElements->data.size() ) {
    entry.dataOwners.push_back( entry );
    exit.dataOwners.push_back( entry );
  }
  
  // populate lookup maps of sequential activities for each performer
  if ( extensionElements->hasSequentialPerformer ) {
    sequentialActivities.emplace(&entry,std::vector< std::pair<const Vertex&, const Vertex&> >());
  }
  else if ( auto sequentialAdHocSubProcess = node->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
    sequentialAdHocSubProcess && !sequentialAdHocSubProcess->performer
  ) {
    sequentialActivities.emplace(&entry,std::vector< std::pair<const Vertex&, const Vertex&> >());
  }

  if ( node->represents<BPMN::Activity>() && extensionElements->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto performerVertices = entry.performer();
    sequentialActivities.at( &performerVertices.first ).push_back( { entry, exit } );
  }

  // populate lookup maps of data modifiers for data owner
  if ( extensionElements->data.size() ) {
    dataModifiers.emplace(&entry,std::vector< std::pair<const Vertex&, const Vertex&> >());
//    dataModifiers.emplace(&exit,std::vector< std::pair<const Vertex&, const Vertex&> >()); // ISTHISNEEDED
  }

  if ( node->represents<BPMN::Task>() ) {
    for ( auto& operator_ : extensionElements->operators ) {
      if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
        auto dataOwnerVertices = entry.dataOwner(operator_->attribute);
        dataModifiers.at( &dataOwnerVertices.first ).push_back( { entry, exit } );
      }
      else if ( operator_->attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL ) {
        globalModifiers.push_back( { entry, exit } );
      }
    }
  }
  
  return { entry, exit };
}

void FlattenedGraph::createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Activity* activity) {
  // loop & multi-instance activties
  
  // lambda returning parameter value known at time zero
  auto getValue = [&](BPMNOS::Model::Parameter* parameter, BPMNOS::ValueType type) -> std::optional<BPMNOS::number> {
    if ( parameter->attribute.has_value() ) {
      BPMNOS::Model::Attribute& attribute = parameter->attribute->get();
      if ( !attribute.isImmutable ) {
        throw std::runtime_error("FlattenedGraph: Loop parameter '" + parameter->name + "' for activity '" + activity->id +"' must be immutable" );
      }
      auto value = scenario->getKnownValue(rootId, &attribute, 0);
      if ( value.has_value() ) {
        return value.value();
      }
    }
      
    if ( parameter->value.has_value() ) {
      return BPMNOS::to_number( parameter->value.value().get(), type);
    }

    return std::nullopt;
  };
  
  int n = 0;
  auto extensionElements = activity->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  assert(extensionElements);

  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    if ( extensionElements->loopMaximum.has_value() ) {
      auto value = getValue( extensionElements->loopMaximum.value().get(), INTEGER ); 
      n = value.has_value() ? (int)value.value() : 0;
    }
  }
  else {
    if ( extensionElements->loopMaximum.has_value() ) {
      auto value = getValue( extensionElements->loopCardinality.value().get(), INTEGER );
      n = value.has_value() ? (int)value.value() : 0;
    }
    
    // determine implicit cardinality from collection size
    auto attributes = extensionElements->attributes | std::views::filter([](auto& attribute) {
      return (attribute->collection != nullptr);
    });

    for ( auto& attribute : attributes ) {
      auto collectionValue = getValue( attribute->collection.get(), COLLECTION );
      if ( !collectionValue.has_value() ) {
        throw std::runtime_error("FlattenedGraph: unable to determine collection for attribute '" + attribute->name + "'");
      }
      auto& collection = collectionRegistry[(long unsigned int)collectionValue.value()].values;
      if ( n > 0 && n != (int)collection.size() ) {
        throw std::runtime_error("FlattenedGraph: inconsistent number of values provided for multi-instance activity '" + activity->id +"'" );
      }
      n = (int)collection.size();
    }
  }
      
  if ( n <= 0 ) {
    throw std::runtime_error("FlattenedGraph: cannot determine loop maximum/cardinality for activity '" + activity->id +"'" );
  }
 
  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    // create vertices for loop activity
    for ( int i = 1; i <= n; i++ ) {
      createVertexPair(rootId, instanceId, activity);
    }
  }
  else {
    std::string baseName = BPMNOS::to_string(instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + activity->id + BPMNOS::Model::Scenario::delimiters[1] ;
    // create vertices for multi-instance activity
    for ( int i = 1; i <= n; i++ ) {
      createVertexPair(rootId, BPMNOS::to_number(baseName + std::to_string(i),STRING), activity);
    }
  }
  
  if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::MultiInstanceParallel ) {
    // create sequential precedences
    auto& container = vertexMap.at(activity).at(instanceId);
    for ( size_t i = 1; i < container.size(); i += 2 ) {
      Vertex& predecessor = container[i]; // exit vertex
      Vertex& successor = container[i+1]; // entry vertex
      predecessor.successors.push_back(successor);
      successor.predecessors.push_back(predecessor);
    }
  }

}

void FlattenedGraph::flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit) {
  for ( auto& flowNode : scope->flowNodes ) {
    
    // create vertices for flow node
    auto activity = flowNode->represents<BPMN::Activity>();
    if ( activity && activity->loopCharacteristics.has_value() ) {
      createLoopVertices(scopeEntry.rootId, instanceId, activity);
    }
    else {
      createVertexPair(scopeEntry.rootId, instanceId, flowNode);
    }
        
    auto& container = vertexMap.at(flowNode).at(instanceId);
    // add predecessors and successors for vertices
    for ( Vertex& vertex : container ) {
      vertex.predecessors.push_back(scopeEntry);
      vertex.successors.push_back(scopeExit);
    }

    // flatten child scopes
    if ( auto childScope = flowNode->represents<BPMN::Scope>() ) {
      assert( container.size() % 2 == 0 );
      for ( size_t i = 0; i < container.size(); i += 2 ) {
        Vertex& entry = container[i];
        Vertex& exit = container[i+1];
        flatten( entry.instanceId, childScope, entry, exit );
      }
    }
  }

  // sequence flows
  for ( auto& sequenceFlow : scope->sequenceFlows ) {
    Vertex& origin = vertexMap.at(sequenceFlow->source).at(instanceId).front();
    Vertex& destination = vertexMap.at(sequenceFlow->target).at(instanceId).back();
    origin.outflows.emplace_back(sequenceFlow.get(),destination);
    destination.inflows.emplace_back(sequenceFlow.get(),origin);
  }

  // boundary events
  for ( auto& flowNode : scope->flowNodes ) {
    if ( auto boundaryEvent = flowNode->represents<BPMN::BoundaryEvent>() ) {
      throw std::runtime_error("FlattenedGraph: Boundary event '" + boundaryEvent->id + "' is not yet supported");
    }    
  }
    
  // event-subprocesses
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    if ( eventSubProcess->startEvent->isInterrupting ) {
      // interrupting event-subprocesses
      flatten( instanceId, eventSubProcess, scopeEntry, scopeExit );
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
      throw std::runtime_error("FlattenedGraph: Compensation activity '" + activity->id + "' is not yet supported");
    }
  } 
}

