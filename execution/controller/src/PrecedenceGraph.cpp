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
    // create process vertices
    auto container = createVertices( instance->id, instance->id, instance->process);
    auto entry = container.front();
    auto exit = container.back();
    initialVertices.push_back(entry);
    flatten( instance->id, instance->process, entry, exit );
  }
  // TODO: message flows
  // std::vector< const BPMN::FlowNode* > ExtensionElements::messageCandidates; 
}

std::vector< PrecedenceGraph::Vertex >& PrecedenceGraph::createLoopVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Activity* activity) {
  // loop & multi-instance activties
  std::vector<Vertex>& container = vertices[activity][instanceId];
  
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
 
  // lambda creating entry and exit vertices
  auto createVertexPair = [&](BPMNOS::number id) -> void {
    container.emplace_back( Vertex(rootId, id, activity, Vertex::Type::ENTRY) );
    auto& entry = container.back();
    container.emplace_back( Vertex(rootId, id, activity, Vertex::Type::EXIT) );
    auto& exit = container.back();
    entry.successors.push_back(exit);
    exit.predecessors.push_back(entry);
  };

  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    // create vertices for loop activity
    for ( int i = 1; i <= n; i++ ) {
      createVertexPair(instanceId);
    }
  }
  else {
    std::string baseName = BPMNOS::to_string(instanceId,STRING) + BPMNOS::Model::Scenario::delimiters[0] + activity->id + BPMNOS::Model::Scenario::delimiters[1] ;
    // create vertices for loop activity
    for ( int i = 1; i <= n; i++ ) {
      createVertexPair( BPMNOS::to_number(baseName + std::to_string(i),STRING) );
    }
  }
  
  if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::MultiInstanceParallel ) {
    // create sequential precedences
    for ( size_t i = 1; i < container.size(); i += 2 ) {
      auto& predecessor = container[i]; // exit vertex
      auto& successor = container[i+1]; // entry vertex
      predecessor.successors.push_back(successor);
      successor.predecessors.push_back(predecessor);
    }
  }
    


  return container;
}

std::vector< PrecedenceGraph::Vertex >& PrecedenceGraph::createVertices(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* node) {
  auto& container = vertices[node].emplace( 
    instanceId, 
    std::vector<Vertex>{ Vertex(rootId, instanceId, node, Vertex::Type::ENTRY), Vertex( rootId, instanceId, node, Vertex::Type::EXIT) }
  ).first->second;
  auto& entry = container.front();
  auto& exit = container.back();
  entry.successors.push_back(exit);
  exit.predecessors.push_back(entry);
  
  return container;
}

void PrecedenceGraph::flatten(BPMNOS::number instanceId, const BPMN::Scope* scope, Vertex& scopeEntry, Vertex& scopeExit) {
  for ( auto& flowNode : scope->flowNodes ) {
    
    // create vertices for flow node
    auto activity = flowNode->represents<BPMN::Activity>();
    
    std::vector<Vertex>& container = 
      (activity && activity->loopCharacteristics.has_value()) ?
      createLoopVertices(scopeEntry.rootId, instanceId, activity) :
      createVertices(scopeEntry.rootId, instanceId, flowNode)
    ;
        
    // add predecessors and successors for vertices
    for ( auto& vertex : container ) {
      vertex.predecessors.push_back(scopeEntry);
      vertex.successors.push_back(scopeExit);
    }

    // flatten child scopes
    if ( auto childScope = flowNode->represents<BPMN::Scope>() ) {
      assert( container.size() % 2 == 0 );
      for ( size_t i = 0; i < container.size(); i += 2 ) {
        auto& entry = container[i];
        auto& exit = container[i+1];
        flatten( entry.instanceId, childScope, entry, exit );
      }
    }
  }

  // sequence flows
  for ( auto& sequenceFlow : scope->sequenceFlows ) {
    auto& origin = vertices.at(sequenceFlow->source).at(instanceId).front();
    auto& destination = vertices.at(sequenceFlow->target).at(instanceId).back();
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
      if ( auto messageStartEvent = eventSubProcess->startEvent->represents<BPMN::MessageStartEvent>() ) {
        // remember non-interrupting event-subprocess to be flattened when number of instantiations is known
        nonInterruptingEventSubProcesses.emplace_back(scopeEntry.rootId, instanceId, eventSubProcess);
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

