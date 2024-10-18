#include "PrecedenceGraph.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/CollectionRegistry.h"
#include <ranges>

using namespace BPMNOS::Execution;

PrecedenceGraph::PrecedenceGraph(const BPMNOS::Model::Scenario* scenario) : scenario(scenario) {
  // get all known instances
  auto instances = scenario->getKnownInstances(0);
  for ( auto& instance : instances ) {
    addInstance( instance );
  }
}

void PrecedenceGraph::addInstance( const BPMNOS::Model::Scenario::InstanceData* instance ) {
  // create process vertices
  auto [ entry, exit ] = createVertexPair(instance->id, instance->id, instance->process);
  initialVertices.push_back(entry);
  flatten( instance->id, instance->process, entry, exit );
}

void PrecedenceGraph::addNonInterruptingEventSubProcess( const BPMN::EventSubProcess* eventSubProcess, Vertex& parentEntry, Vertex& parentExit ) {
  nonInterruptingEventSubProcesses.emplace_back(eventSubProcess, parentEntry, parentExit, 0);
  // iterate through all known trigger and flatten event-subprocess instantiations
  auto& counter = std::get<unsigned int>( nonInterruptingEventSubProcesses.back() );
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
    }
  } 
}

void PrecedenceGraph::addSender( const BPMN::MessageThrowEvent* messageThrowEvent, Vertex& senderEntry, Vertex& senderExit ) {
  // flatten event subprocesses if applicable
  assert( messageThrowEvent->extensionElements );
  assert( messageThrowEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageThrowEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto& [eventSubProcess, parentEntry, parentExit, counter] : nonInterruptingEventSubProcesses ) {
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
      senderEntry.successors.push_back(recipientExit);
      recipientExit.predecessors.push_back(senderEntry);
      if ( messageThrowEvent->represents<BPMN::SendTask>() ) {
        recipientExit.successors.push_back(senderExit);
        senderExit.predecessors.push_back(recipientExit);
      }
    }
  }
  
  sendingVertices[messageThrowEvent].emplace_back(senderEntry, senderExit);
}

void PrecedenceGraph::addRecipient( const BPMN::MessageCatchEvent* messageCatchEvent, Vertex& recipientEntry, Vertex& recipientExit ) {
  // set precedences for all senders
  assert( messageCatchEvent->extensionElements );
  assert( messageCatchEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto& candidates = messageCatchEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
  for ( auto candidate : candidates ) {
    for ( auto& [ senderEntry, senderExit] : sendingVertices[candidate] ) {
      senderEntry.successors.push_back(recipientExit);
      recipientExit.predecessors.push_back(senderEntry);
      if ( candidate->represents<BPMN::SendTask>() ) {
        recipientExit.successors.push_back(senderExit);
        senderExit.predecessors.push_back(recipientExit);
      }
    }
  }

  receivingVertices[messageCatchEvent].emplace_back(recipientEntry, recipientExit);
}

std::pair<PrecedenceGraph::Vertex&, PrecedenceGraph::Vertex&> PrecedenceGraph::createVertexPair(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node) {
  vertices.emplace_back(rootId, instanceId, node, Vertex::Type::ENTRY);
  auto& entry = vertices.back();
  vertices.emplace_back(rootId, instanceId, node, Vertex::Type::EXIT);
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

  return { entry, exit };
}



void PrecedenceGraph::createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Activity* activity) {
  // loop & multi-instance activties
  
  // lambda returning parameter value known at time zero
  auto getValue = [&](BPMNOS::Model::Parameter* parameter, BPMNOS::ValueType type) -> std::optional<BPMNOS::number> {
    if ( parameter->attribute.has_value() ) {
      BPMNOS::Model::Attribute& attribute = parameter->attribute->get();
      if ( !attribute.isImmutable ) {
        throw std::runtime_error("PrecedenceGraph: Loop parameter '" + parameter->name + "' for activity '" + activity->id +"' must be immutable" );
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
        throw std::runtime_error("PrecedenceGraph: unable to determine collection for attribute '" + attribute->name + "'");
      }
      auto& collection = collectionRegistry[(long unsigned int)collectionValue.value()].values;
      if ( n > 0 && n != (int)collection.size() ) {
        throw std::runtime_error("PrecedenceGraph: inconsistent number of values provided for multi-instance activity '" + activity->id +"'" );
      }
      n = (int)collection.size();
    }
  }
      
  if ( n <= 0 ) {
    throw std::runtime_error("PrecedenceGraph: cannot determine loop maximum/cardinality for activity '" + activity->id +"'" );
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

void PrecedenceGraph::flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit) {
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
    origin.successors.push_back(destination);
    destination.predecessors.push_back(origin);
  }

  // boundary events
  for ( auto& flowNode : scope->flowNodes ) {
    if ( auto boundaryEvent = flowNode->represents<BPMN::BoundaryEvent>() ) {
      throw std::runtime_error("PrecedenceGraph: Boundary event '" + boundaryEvent->id + "' is not yet supported");
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
        throw std::runtime_error("PrecedenceGraph: Type of non-interrupting event-subprocess '" + eventSubProcess->id + "' is not supported");
      }
    }
  }
  
  // compensation nodes
  for ( auto& flowNode : scope->flowNodes ) {
    auto activity = flowNode->represents<BPMN::Activity>();
    if ( activity && activity->isForCompensation ) {
      throw std::runtime_error("PrecedenceGraph: Compensation activity '" + activity->id + "' is not yet supported");
    }
  } 
}

