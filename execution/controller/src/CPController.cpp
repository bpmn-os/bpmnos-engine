#include "CPController.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"

#include <iostream>

using namespace BPMNOS::Execution;

CPController::CPController()
 : model(CP::Model::ObjectiveSense::MAXIMIZE)
{
/*
  // add event dispatcher
  eventDispatchers.push_back( std::make_unique<BestFirstParallelEntry>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestFirstExit>(evaluator) );
  eventDispatchers.push_back( std::make_unique<RandomChoice>() ); // TODO: replace with  dispatcher returning best choice
  eventDispatchers.push_back( std::make_unique<BestFirstSequentialEntry>(evaluator) );
  eventDispatchers.push_back( std::make_unique<BestMatchingMessageDelivery>(evaluator) );
*/
}

void CPController::connect(Mediator* mediator) {
/*
  for ( auto& eventDispatcher : eventDispatchers ) {
    eventDispatcher->connect(this);
  }
*/
  Controller::connect(mediator);
}

std::shared_ptr<Event> CPController::dispatchEvent(const SystemState* systemState) {
  std::shared_ptr<Decision> best = nullptr;
/*
  for ( auto& eventDispatcher : eventDispatchers ) {
    if ( auto event = eventDispatcher->dispatchEvent(systemState) ) {
      if (  auto decision = dynamic_pointer_cast<Decision>(event) ) {
//std::cerr << decision->jsonify() << std::endl;
        if ( decision->evaluation.has_value() ) {
          if ( !best ) {
            // first feasible decision is used as best
            best = decision;
          }
          else if ( decision->evaluation.value() < best->evaluation.value() ) {
            // decision has less costly evaluation than current best
            best = decision;
          }
        }
      }
      else {
        // events are immediately forwarded
        return event;
      }
    }
  }
*/
  return best;
}

const CP::Model& CPController::createCP(const BPMNOS::Model::Scenario* scenario) {
  this->scenario = scenario;

  // create global variables
  for ( auto& [name,attribute] : scenario->model->attributeRegistry.globalAttributes ) {
    addGlobalVariable( attribute, scenario->globals[attribute->index] );
  }

  instances = scenario->getKnownInstances(0); // get instances at time zero
  for ( auto instance : instances ) {
    if ( !instance->process->isExecutable ) {
      throw std::runtime_error("CPController: process is not executable");
    }

    // register all message sources
    std::vector< const BPMN::Node* >  messageThrowEvents = instance->process->find_all([](const BPMN::Node* node) {  return node->represents<BPMN::MessageThrowEvent>(); } );
    for ( auto messageThrowEvent : messageThrowEvents ) {
      originInstances[messageThrowEvent->as<BPMN::MessageThrowEvent>()].push_back(instance->id);
    }
  }

  for ( auto instance : instances ) {
    auto reference = NodeReference{instance->id,instance->process};
    addEntryVariables(reference);
    pendingExit.push_back(reference);
  }
  
  while ( pendingEntry.size() || pendingExit.size() ) {
    bool cycle = true;

    for (auto it = pendingEntry.begin(); it != pendingEntry.end(); ) {
      if ( checkEntryDependencies(*it) ) {
        addEntryVariables(*it);
        if ( auto scope = it->second->represents<BPMN::Scope>() ) {
          addChildren(ScopeReference{it->first,scope});
        }
        pendingExit.push_back(*it);
        cycle = false;
        it = pendingEntry.erase(it);
      }
      else {
        it++;
      }
    }

    for (auto it = pendingExit.begin(); it != pendingExit.end(); ) {
      if ( checkExitDependencies(*it) ) {
        addExitVariables(*it);
        if ( it->second->represents<BPMN::FlowNode>() ) {
          addSuccessors(*it);
        }
        cycle = false;
        it = pendingExit.erase(it);
      }
      else {
        it++;
      }
    }
    if ( cycle ) {
      throw std::runtime_error("CPController: model has cyclic dependency");
    }
  }
  
  return model;
}

void CPController::addGlobalVariable( const BPMNOS::Model::Attribute* attribute, std::optional<BPMNOS::number> value ) {
  if ( value.has_value() ) {
    globals.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + attribute->name, (double)true,(double)true ), 
      model.addVariable(CP::Variable::Type::REAL, "value_" + attribute->name, (double)value.value(), (double)value.value()) 
    );
  }
  else {
    globals.emplace_back(
      model.addVariable(CP::Variable::Type::BOOLEAN, "defined_" + attribute->name, (double)false,(double)false ), 
      model.addVariable(CP::Variable::Type::REAL, "value_" + attribute->name, 0.0, 0.0) 
    );
  }
}

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
/*
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
*/
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

/*
void CPController::createSequenceFlowTraversalVariables(SequenceFlowReference reference) {
}
*/

///  TODO: ensure that the highest entry data state remains smaller than the sum of sequential activities beeing used 
///  TODO: ensure that the highest exit data state remains smaller or equal than the sum of sequential activities beeing used 
///  TODO: add constraint that the the sum of entry data state variables for each sequential activity and each data state is one

