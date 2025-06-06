#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "DecisionRequest.h"
#include "SequentialPerformerUpdate.h"
#include "DataUpdate.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Gatekeeper.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include "model/bpmnos/src/extensionElements/Signal.h"
#include "model/bpmnos/src/extensionElements/Conditions.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/utility/src/erase.h"
#include "model/utility/src/Number.h"
#include <cassert>
#include <iostream>

using namespace BPMNOS::Execution;

Token::Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status)
  : owner(owner)
  , owned(nullptr)
  , node(node)
  , sequenceFlow(nullptr)
  , state(State::CREATED)
  , status(status)
  , data(&const_cast<StateMachine*>(owner)->data)
  , globals(const_cast<SystemState*>(owner->systemState)->globals)
  , performing(nullptr)
{
}

Token::Token(const Token* other)
  : owner(other->owner)
  , owned(nullptr)
  , node(other->node)
  , sequenceFlow(nullptr)
  , state(other->state)
  , status(other->status)
  , data(&const_cast<StateMachine*>(owner)->data)
  , globals(const_cast<SystemState*>(owner->systemState)->globals)
  , performing(nullptr)
{
}

Token::Token(const std::vector<Token*>& others)
  : owner(others.front()->owner)
  , owned(nullptr)
  , node(others.front()->node)
  , sequenceFlow(nullptr)
  , state(others.front()->state)
  , status(mergeStatus(others))
  , data(&const_cast<StateMachine*>(owner)->data)
  , globals(const_cast<SystemState*>(owner->systemState)->globals)
  , performing(nullptr)
{
}

Token::~Token() {
//std::cerr << "~Token(" << (node ? node->id : owner->process->id ) << "/" << this << ")" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  if ( node) {
    if ( auto activity = node->represents<BPMN::Activity>(); activity && !activity->boundaryEvents.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      auto stateMachine = const_cast<StateMachine*>(owner);
      engine->commands.emplace_back(std::bind(&StateMachine::deleteTokensAwaitingBoundaryEvent,stateMachine,this), this);
    }

    if ( node->represents<BPMN::BoundaryEvent>() ) {
      systemState->tokenAssociatedToBoundaryEventToken.erase(this);
    }

    if ( node->represents<BPMN::CompensateThrowEvent>() ||
         node->represents<BPMN::CompensateBoundaryEvent>() ||
         node->represents<BPMN::CompensateStartEvent>() ||
         node->represents<BPMN::Activity>() 
    ) {
      systemState->tokenAwaitingCompensationActivity.erase(this);
    }

    if ( node->represents<BPMN::EventBasedGateway>() ) {
      systemState->tokensAwaitingEvent.erase(this);
    }
    if ( node->represents<BPMN::CatchEvent>() ) {
      systemState->tokenAtEventBasedGateway.erase(this);
    }

    if ( auto activity = node->represents<BPMN::Activity>() ) {
      if ( activity->represents<BPMN::SendTask>() ) {
        auto it = systemState->messageAwaitingDelivery.find(this);
        if ( it != systemState->messageAwaitingDelivery.end() ) {
          auto message = it->second.lock();
          if ( message ) {
            // withdraw message
            message->state = Message::State::WITHDRAWN;
            owner->systemState->engine->notify(message.get());            
            erase_ptr<Message>(systemState->messages, message.get());
          }
          systemState->messageAwaitingDelivery.erase(it);
        }
      }

      // release performer when activity fails
      if ( state == State::READY && activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        auto tokenAtSequentialPerformer = getSequentialPerformerToken();
        if ( tokenAtSequentialPerformer && tokenAtSequentialPerformer->performing == this ) {
          releaseSequentialPerformer();
        }
      }

      if ( activity->loopCharacteristics.has_value() &&
        !activity->isForCompensation
      ) {
        if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard ) {
          if ( state == State::WAITING ) {
            systemState->tokensAtActivityInstance.erase(this);
            systemState->exitStatusAtActivityInstance.erase(this);
          }
          else {
            systemState->tokenAtMultiInstanceActivity.erase(this);
            if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
              systemState->tokenAwaitingMultiInstanceExit.erase(this);
            }
          }
        }
      }
    }
    
    if ( performing ) {
      performing = nullptr;
      owner->systemState->engine->notify(SequentialPerformerUpdate(this));
    }
  }
}


const BPMNOS::Model::AttributeRegistry& Token::getAttributeRegistry() const {
  if ( !node ) {
    return owner->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  }

  if ( auto extensionElements = node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>() ) {
    return extensionElements->attributeRegistry;
  }

  // return attribute registry of parent for nodes without extension elements
  if ( !owner->parentToken ) {
    throw std::runtime_error("Token: cannot determine attribute registry");
  }

  return owner->parentToken->getAttributeRegistry();
}

BPMNOS::number Token::getInstanceId() const {
  return (*data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value();
}

nlohmann::ordered_json Token::jsonify() const {
  nlohmann::ordered_json jsonObject;
  jsonObject["processId"] = owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string(getInstanceId(),STRING);
  if ( node ) {
    jsonObject["nodeId"] = node->id;
  }
  if ( sequenceFlow ) {
    jsonObject["sequenceFlowId"] = sequenceFlow->id;
  }
  jsonObject["state"] = stateName[(int)state];
  jsonObject["status"] = nlohmann::ordered_json::object();

  auto& attributeRegistry = getAttributeRegistry();
  for (auto attribute : attributeRegistry.statusAttributes ) {
    if ( attribute->index >= status.size() ) {
      // skip attribute that is not yet included in status
      continue;
    }

    auto statusValue = attributeRegistry.getValue(attribute,status,*data,globals);
    if ( !statusValue.has_value() ) {
      jsonObject["status"][attribute->name] = nullptr ;
    }
    else if ( attribute->type == BOOLEAN) {
      bool value = (bool)statusValue.value();
      jsonObject["status"][attribute->name] = value ;
    }
    else if ( attribute->type == INTEGER) {
      int value = (int)statusValue.value();
      jsonObject["status"][attribute->name] = value ;
    }
    else if ( attribute->type == DECIMAL) {
      double value = (double)statusValue.value();
      jsonObject["status"][attribute->name] = value ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(statusValue.value(),attribute->type);
      jsonObject["status"][attribute->name] = value ;
    }
    else if ( attribute->type == COLLECTION) {
      std::string value = BPMNOS::to_string(statusValue.value(),attribute->type);
      jsonObject["status"][attribute->name] = value ;
    }
  }

//std::cerr << jsonObject << std::endl;
  assert(data);
  if ( data->size() ) {
    jsonObject["data"] = nlohmann::ordered_json::object();

    for (auto attribute : attributeRegistry.dataAttributes ) {
      if ( attribute->index >= data->size() ) {
        // skip attribute that is not yet included in data
        continue;
      }

      auto dataValue = attributeRegistry.getValue(attribute,status,*data,globals);
      if ( !dataValue.has_value() ) {
        jsonObject["data"][attribute->name] = nullptr ;
      }
      else if ( attribute->type == BOOLEAN) {
        bool value = (bool)dataValue.value();
        jsonObject["data"][attribute->name] = value ;
      }
      else if ( attribute->type == INTEGER) {
        int value = (int)dataValue.value();
        jsonObject["data"][attribute->name] = value ;
      }
      else if ( attribute->type == DECIMAL) {
        double value = (double)dataValue.value();
        jsonObject["data"][attribute->name] = value ;
      }
      else if ( attribute->type == STRING) {
        std::string value = BPMNOS::to_string(dataValue.value(),attribute->type);
        jsonObject["data"][attribute->name] = value ;
      }
      else if ( attribute->type == COLLECTION) {
        std::string value = BPMNOS::to_string(dataValue.value(),attribute->type);
        jsonObject["data"][attribute->name] = value ;
      }
    }
  }

  if ( globals.size() ) {
    jsonObject["globals"] = nlohmann::ordered_json::object();

    for (auto attribute : attributeRegistry.globalAttributes ) {
      auto globalValue = globals[attribute->index];
      if ( !globalValue.has_value() ) {
        jsonObject["globals"][attribute->name] = nullptr ;
      }
      else if ( attribute->type == BOOLEAN) {
        bool value = (bool)globalValue.value();
        jsonObject["globals"][attribute->name] = value ;
      }
      else if ( attribute->type == INTEGER) {
        int value = (int)globalValue.value();
        jsonObject["globals"][attribute->name] = value ;
      }
      else if ( attribute->type == DECIMAL) {
        double value = (double)globalValue.value();
        jsonObject["globals"][attribute->name] = value ;
      }
      else if ( attribute->type == STRING) {
        std::string value = BPMNOS::to_string(globalValue.value(),attribute->type);
        jsonObject["globals"][attribute->name] = value ;
      }
      else if ( attribute->type == COLLECTION) {
        std::string value = BPMNOS::to_string(globalValue.value(),attribute->type);
        jsonObject["globals"][attribute->name] = value ;
      }
    }
  }
  
  return jsonObject;
}


bool Token::entryIsFeasible() const {
  if ( !node ) {
//std::cerr << stateName[(int)state] << "/" << owner->scope->id <<std::endl;
    assert( owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    return owner->process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleEntry(status,*data,globals);
  }

  assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  return node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleEntry(status,*data,globals);
}

bool Token::exitIsFeasible() const {
  if ( !node ) {
//std::cerr << stateName[(int)state] << "/" << owner->scope->id <<std::endl;
    assert( owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    return owner->process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleExit(status,*data,globals);
  }

  assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  return node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleExit(status,*data,globals);
}

void Token::advanceFromCreated() {
//std::cerr << "advanceFromCreated: " << jsonify().dump() << std::endl;
  if ( !node ) {
    // tokens at process advance to entered
    advanceToEntered();
    return;
  }

  // token is at a node
  if ( node->represents<BPMN::Activity>() ) {
    awaitReadyEvent();
  }
  else {
    advanceToEntered();
  }
}

void Token::advanceToReady() {
//std::cerr << "advanceToReady: " << jsonify().dump() << std::endl;
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: ready timestamp at node '" + node->id + "' is larger than current time");
  }

  if ( owned ) {
    // ensure that data is set appropriately
    data = &owned->data;
  }
    
  update(State::READY);
  
  if ( auto activity = node->represents<BPMN::Activity>();
    activity && 
    activity->loopCharacteristics.has_value()
  ) {
    if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard ) {
      // delegate creation of token copies for multi-instance activity to owner
      auto stateMachine = const_cast<StateMachine*>(owner);
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::createMultiInstanceActivityTokens,stateMachine,this), this);
      return;
    }
  }

//std::cerr << "->awaitEntryEvent" << std::endl;
  awaitEntryEvent();
}

void Token::computeInitialValues( const BPMNOS::Model::ExtensionElements* extensionElements ) {
  extensionElements->computeInitialValues(owner->systemState->currentTime,status,*data,globals);
}

void Token::advanceToEntered() {
//std::cerr << "advanceToEntered: " << jsonify().dump() << std::endl;

  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    if ( node ) {
      throw std::runtime_error("Token: entry timestamp at node '" + node->id + "' is larger than current time");
    }
    else {
      throw std::runtime_error("Token: entry timestamp for process '" + owner->process->id + "' is larger than current time");
    }
  }

  if ( !node ) {
//std::cerr << "!node" << std::endl;
    auto extensionElements = owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
    assert( extensionElements );

    computeInitialValues( extensionElements );
  }
  else {
    if ( auto activity = node->represents<BPMN::Activity>() ) {
      auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
      assert( extensionElements );

      if ( 
        activity->loopCharacteristics.has_value() && 
        activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard
      ) {
//std::cerr << "initialize or increment loop index for standard loop" << std::endl;
        // initialize or increment loop index for standard loop
        if ( extensionElements->loopIndex.has_value() && extensionElements->loopIndex.value()->expression ) {
          auto& attributeRegistry = getAttributeRegistry();
          auto attribute = extensionElements->loopIndex.value()->expression->isAttribute();
          if ( auto index = attributeRegistry.getValue( attribute, status, *data, globals); index.has_value() ) {
            // increment existing value 
            attributeRegistry.setValue(attribute, status, *data, globals, (unsigned int)index.value() + 1);
          }
          else {
            // initialize non-existing value 
            attributeRegistry.setValue(attribute, status, *data, globals, 1);
          }
        }
        else if ( extensionElements->loopMaximum.has_value() ) {
          throw std::runtime_error("Token: no attribute provided for loop index parameter of standard loop activity '" + node->id +"' with loop maximum" );
        }
      }
      
      computeInitialValues( extensionElements );
//std::cerr << jsonify() << std::endl;
    }    
  }

  
  update(State::ENTERED);
//std::cerr << "updatedToEntered" << std::endl;

  if ( const BPMN::Activity* activity = (node ? node->represents<BPMN::Activity>() : nullptr);
    activity && 
    activity->loopCharacteristics.has_value() &&
    owned
  ) {
    // register state machines of multi-instance activities
    const_cast<SystemState*>(owner->systemState)->archive[ (long unsigned int)owned->instance.value() ] = owned->weak_from_this();
    owned->registerRecipient();
  }

//std::cerr << jsonify().dump() << std::endl;

  auto engine = const_cast<Engine*>(owner->systemState->engine);


  if ( node && node->represents<BPMN::UntypedStartEvent>() ) {
    // initiate event subprocesses after entering untyped start event
    auto stateMachine = const_cast<StateMachine*>(owner);
    engine->commands.emplace_back(std::bind(&StateMachine::initiateEventSubprocesses,stateMachine,this), this);
  }


  // only check feasibility for processes and activities
  // feasibility of all other tokens must have been validated before
  // (also for newly created or merged tokens)
  if ( !node ) {
    // check restrictions
    if ( !entryIsFeasible() ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
      return;
    }

    // tokens entering a process advance to busy state
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), this);
  }
  else if ( auto activity = node->represents<BPMN::Activity>() ) {
//std::cerr << "activity" << std::endl;

    auto stateMachine = const_cast<StateMachine*>(owner);

    // create tokens at boundary events
    if ( activity->boundaryEvents.size() ) {
      engine->commands.emplace_back(std::bind(&StateMachine::initiateBoundaryEvents,stateMachine,this), this);
    }

//std::cerr << "check restrictions: " << entryIsFeasible() << std::endl;
    // check restrictions
    if ( !entryIsFeasible() ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
      return;
    }

    // advance to busy state
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), this);
  }
  else if ( node->represents<BPMN::CatchEvent>() && !node->represents<BPMN::UntypedStartEvent>()
  ) {
//std::cerr << "advance to busy state" << std::endl;
    // tokens entering a catching event automatically
    // advance to busy state
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), this);
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    // tokens entering an event-based gateway automatically
    // advance to busy state
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), this);
  }
  else if ( node->represents<BPMN::ErrorEndEvent>() ) {
    engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
    return;
  }
  else {
    if ( node->represents<BPMN::EscalationThrowEvent>() ) {
      // update status and delegate control to state machine
      // if applicable, control will be delgated back to token
      engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,const_cast<StateMachine*>(owner),this), this);
    }
    else if ( node->represents<BPMN::SignalThrowEvent>() ) {
      // determine signal name
      emitSignal();
    }
    else if ( node->represents<BPMN::MessageThrowEvent>() ) {
      assert( !node->represents<BPMN::SendTask>() );
      sendMessage();
    }
    else if ( auto compensateThrowEvent = node->represents<BPMN::CompensateThrowEvent>() ) {
      auto context = const_cast<StateMachine*>(owner->parentToken->owned.get());

      if ( auto eventSubProcess = node->parent->represents<BPMN::EventSubProcess>();
        eventSubProcess && eventSubProcess->startEvent->represents<BPMN::CompensateStartEvent>()
      ) {
//std::cerr << "try to update context " << context->compensableSubProcesses.size() << std::endl;
        // compensation is triggered from within a compensation event subprocess
        // find the compensable subprocess and update context
        auto it = std::find_if(
          context->compensableSubProcesses.begin(),
          context->compensableSubProcesses.end(),
          [&eventSubProcess](const std::shared_ptr<StateMachine>& stateMachine) -> bool {
            // check if compensation event subprocess belongs to compensable subprocess
            return ( stateMachine->scope->compensationEventSubProcess == eventSubProcess );
          }
        );
        context = ( it != context->compensableSubProcesses.end() ? it->get() : nullptr );
      }

      if ( context ) {
//std::cerr << "compensate context " << context->scope->id << std::endl;
        if ( auto compensations = context->getCompensationTokens(compensateThrowEvent->activity);
          compensations.size()
        ) {
          // advance to busy and await compensations
          engine->commands.emplace_back(std::bind(&Token::update,this,State::BUSY), this);
          context->compensate( std::move(compensations), this );
          return;
        }
      }
      // nothing to compensate, continue with token flow
    }

    // tokens entering any other node automatically advance to done or
    // departed state
    if ( node->outgoing.empty() ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), this);
      return;
    }
    advanceToDeparting();
  }
}

void Token::advanceToBusy() {
//std::cerr << "advanceToBusy: " << jsonify().dump() << std::endl;

  if ( 
      node &&
      node->represents<BPMN::Task>()
      && !node->represents<BPMN::ReceiveTask>()
      && !node->represents<BPMNOS::Model::DecisionTask>()
    ) {
    if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
       extensionElements && extensionElements->operators.size()
    ) {
      // apply operators for regular tasks (if timestamp is in the future, updated status is an expectation)
      auto now = owner->systemState->getTime();
      status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = now;
      extensionElements->applyOperators(status,*data,globals);
      if ( !status[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() ) {
        throw std::runtime_error("Token: timestamp at node '" + node->id + "' is deleted");
      }
      if ( node->represents<BPMN::SendTask>() && status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() != now) {
std::cerr << status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() << " != " << now << std::endl;
        throw std::runtime_error("Token: Operators for task '" + node->id + "' attempt to modify timestamp");
      }
      // notify about data update
      if ( extensionElements->dataUpdate.global ) {
        owner->systemState->engine->notify( DataUpdate( extensionElements->dataUpdate.attributes ) );
      }
      else {
        owner->systemState->engine->notify( DataUpdate( owner->root->instance.value(), extensionElements->dataUpdate.attributes ) );
      }
    }
  }
    
  update(State::BUSY);

  if ( !node ) {
    // token is at process
    auto scope = owner->process->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
    else {
      if ( scope->startNodes.size() == 1 ) {
//std::cerr << "create child statemachine" << std::endl;
        // create child statemachine
        auto engine = const_cast<Engine*>(owner->systemState->engine);
        assert( owned );
        engine->commands.emplace_back(std::bind(&StateMachine::run,owned.get(),status), owned.get());
      }
      else {
        throw std::runtime_error("Token: process '" + scope->id + "' has multiple start nodes");
      }
      return;
    }
  }
  else if ( node->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto scope = node->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
    else {
      // create child statemachine
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      assert( owned );
      engine->commands.emplace_back(std::bind(&StateMachine::run,owned.get(),status), owned.get());
    }
  }
  else if ( node->represents<BPMN::SubProcess>() ) {
    auto scope = node->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
//std::cerr << "skip child statemachine at node " << node->id << std::endl;
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
    else {
      if ( scope->startNodes.size() == 1 ) {
//std::cerr << "create child statemachine at node " << node->id << std::endl;
        // create child statemachine
        auto engine = const_cast<Engine*>(owner->systemState->engine);
        assert( owned );
        engine->commands.emplace_back(std::bind(&StateMachine::run,owned.get(),status), owned.get());
      }
      else {
        throw std::runtime_error("Token: subprocess '" + scope->id + "' has multiple start nodes");
      }
      return;
    }
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    // token will automatically be copied and forwarded along each sequence flow
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparting,this), this);
  }
  else if ( node->represents<BPMN::TimerCatchEvent>() ) {
    // determine time
    auto trigger = node->extensionElements->as<BPMNOS::Model::Timer>()->trigger.get();
    if (!trigger->expression) {
      throw std::runtime_error("Token: no trigger given for node '" + node->id + "'");
    }
    BPMNOS::number time = trigger->expression->execute(status, *data, globals).value_or(owner->systemState->getTime());

    if ( time > owner->systemState->getTime() ) {
      awaitTimer(time);
    }
    else {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
  }
  else if ( node->represents<BPMN::SignalCatchEvent>() ) {
    // determine signal name
    assert( node->extensionElements->represents<BPMNOS::Model::Signal>() );
    awaitSignal( node->extensionElements->as<BPMNOS::Model::Signal>()->name );
  }
  else if ( node->represents<BPMN::ConditionalCatchEvent>() ) {
    // determine conditions
    assert(node->extensionElements->represents<BPMNOS::Model::Conditions>());
    auto extensionElements = node->extensionElements->as<BPMNOS::Model::Conditions>();
    if ( !extensionElements->conditionsSatisfied(status,*data,globals) ) {
      awaitConditions( owner->root->instance.value() );
    }
    else {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
  }
  else if ( node->represents<BPMN::MessageCatchEvent>() ) {
    awaitMessageDelivery();
  }
  else if ( node->represents<BPMN::Task>() ) {
    if ( node->represents<BPMNOS::Model::DecisionTask>() ) {
      awaitChoiceEvent();
      return;
    }

    if ( auto sendTask = node->represents<BPMN::SendTask>() ) {
      assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      // send message(s)
      if ( sendTask->loopCharacteristics.has_value() ) {
        // multi-instance send task requires index to access respective message definition
        if ( !extensionElements->loopIndex.has_value() || !extensionElements->loopIndex->get()->expression ) {
          throw std::runtime_error("Token: send task '" + sendTask->id + "' requires status attribute holding loop index");
        }
        auto attribute = extensionElements->loopIndex->get()->expression->isAttribute();
        if ( !status[attribute->index].has_value() ) { 
          throw std::runtime_error("Token: cannot find loop index for send task '" + sendTask->id + "'");
        }
        assert( status[attribute->index].value() >= 1 );
        sendMessage( (size_t)(int)status[attribute->index].value()-1 );
      }
      else {
        sendMessage();
      }
      // wait for delivery
      return;
    }
    if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) { 
      awaitTaskCompletionEvent();
      return;
    }

    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
  }
}

/*
void Token::advanceToCompleted(const Values& statusUpdate) {
  status = statusUpdate;
  advanceToCompleted();
}
*/

void Token::advanceToCompleted() {
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    if ( node ) {
      throw std::runtime_error("Token: completion timestamp at node '" + node->id + "' is larger than current time");
    }
    else {
      throw std::runtime_error("Token: completion timestamp for process '" + owner->process->id + "' is larger than current time");
    }
  }

  status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = owner->systemState->getTime();
  
  if ( node ) {
    // operators of send task, receive task, and decision task are applied on completion
    if ( 
      node->represents<BPMN::ReceiveTask>() ||
      node->represents<BPMNOS::Model::DecisionTask>() 
    ) {
      if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
        if ( !extensionElements->isInstantaneous ) {
          throw std::runtime_error("Token: Operators for task '" + node->id + "' attempt to modify timestamp");
        }
        extensionElements->applyOperators(status,*data,globals);
        // notify about data update
        if ( extensionElements->dataUpdate.global ) {
          owner->systemState->engine->notify( DataUpdate( extensionElements->dataUpdate.attributes ) );
        }
        else {
          owner->systemState->engine->notify( DataUpdate( owner->root->instance.value(), extensionElements->dataUpdate.attributes ) );
        }
      }
    }
  }


  update(State::COMPLETED);

  auto engine = const_cast<Engine*>(owner->systemState->engine);

  if ( !node ) {
    // update global objective
    assert( owner->scope->extensionElements->as<BPMNOS::Model::ExtensionElements>() );
    const_cast<SystemState*>(owner->systemState)->contributionsToObjective += owner->scope->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getContributionToObjective(status,*data,globals);

//std::cerr << "check restrictions" << std::endl;
  // check restrictions
    if ( !exitIsFeasible() ) {
//std::cerr << "infeasible: " << jsonify().dump() <<  std::endl;
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
      return;
    }
  }
  else {
    if ( auto activity = node->represents<BPMN::Activity>() ) {
      if ( activity->compensatedBy ) {
        if ( activity->isForCompensation ) {
          throw std::runtime_error("Token: compensation activity '" + activity->id + "' must not be compensated"); 
        }
        awaitCompensation();
      }

//std::cerr << activity->id << " is for compensation: " << activity->isForCompensation << std::endl;
      if ( activity->isForCompensation ) {
        // final state for compensation activity reached
        if ( !exitIsFeasible() ) {
          engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
          return;
        }

        auto stateMachine = const_cast<StateMachine*>(owner);
        engine->commands.emplace_back(std::bind(&StateMachine::completeCompensationActivity,stateMachine,this), this);
        // update global objective
        assert( node->extensionElements->as<BPMNOS::Model::ExtensionElements>() );
        const_cast<SystemState*>(owner->systemState)->contributionsToObjective += node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getContributionToObjective(status,*data,globals);
      }
      else {
        awaitExitEvent();
      }
      return;
    }
    else if ( auto compensateBoundaryEvent = node->represents<BPMN::CompensateBoundaryEvent>(); compensateBoundaryEvent ) {
//assert( owner->parentToken->node->represents<BPMN::Scope>() );    
//std::cerr << "token is compensateBoundaryEvent: " << node->id << "/" << stateName[(int)state] << "/" << ( owner->parentToken->node ? owner->parentToken->node->id : owner->scope->id) << "/" << this  << "/" << owner <<std::endl;
      engine->commands.emplace_back(std::bind(&StateMachine::compensateActivity,const_cast<StateMachine*>(owner),this), this);
      return;
    }
    else if ( auto boundaryEvent = node->represents<BPMN::BoundaryEvent>() ) {
      auto stateMachine = const_cast<StateMachine*>(owner);
      auto tokenAtActivity = const_cast<SystemState*>(owner->systemState)->tokenAssociatedToBoundaryEventToken[this];
      erase_ptr<Token>(const_cast<SystemState*>(owner->systemState)->tokensAwaitingBoundaryEvent[tokenAtActivity],this);

      if ( boundaryEvent->isInterrupting ) {
        // interrupt activity
        engine->commands.emplace_back(std::bind(&StateMachine::interruptActivity,stateMachine,tokenAtActivity), tokenAtActivity);
      }
      else {
        // create new token at boundary event
        engine->commands.emplace_back(std::bind(&StateMachine::initiateBoundaryEvent,stateMachine,tokenAtActivity,node), tokenAtActivity);
      }
    }
    else if ( node->represents<BPMN::CompensateStartEvent>() ) {
      // nothing do
    } 
    else if ( auto startEvent = node->represents<BPMN::TypedStartEvent>() ) {
      // event subprocess is triggered
      if ( !node->parent->represents<BPMN::EventSubProcess>() ) {
        throw std::runtime_error("Token: typed start event must belong to event subprocess");
      }
      auto context = const_cast<StateMachine*>(owner->parentToken->owned.get());

/*
std::cerr << "Node: " << this << " at " << node->id << " is owned by " << owner << std::endl;
std::cerr << "Owner: " << owner << " at " << owner->scope->id << " has " << owner->pendingEventSubProcesses.size() << " pendingEventSubProcess" << std::endl;
std::cerr << "Context: " << context << " at " << context->scope->id << " has " << context->pendingEventSubProcesses.size() << " pendingEventSubProcess" << std::endl;
*/
      // find pending subprocess
      auto it = std::find_if(context->pendingEventSubProcesses.begin(), context->pendingEventSubProcesses.end(), [this](std::shared_ptr<StateMachine>& eventSubProcess) {
        auto pendingToken = eventSubProcess->tokens.front();
        return pendingToken.get() == this;
      });

      assert( it != context->pendingEventSubProcesses.end() );

      if ( startEvent->isInterrupting ) {
        // TODO: interrupt activity or process

        // move triggered event subprocess to interruptingEventSubProcess
        context->interruptingEventSubProcess = std::move(*it);
        context->pendingEventSubProcesses.erase(it);

        // ensure that no other event subprocess can be triggered
        for ( auto eventSubProcess : context->pendingEventSubProcesses ) {
          eventSubProcess->clearObsoleteTokens();
        }
        context->pendingEventSubProcesses.clear();

        // terminate all running non-interrupting event subprocesses
        for ( auto eventSubProcess : context->nonInterruptingEventSubProcesses ) {
          eventSubProcess->clearObsoleteTokens();
        }
        context->nonInterruptingEventSubProcesses.clear();

        // interrupt all running tokens in state machine
        context->clearObsoleteTokens();
      }
      else {
//std::cerr << "Before pendingEventSubProcesses: " << context->pendingEventSubProcesses.size() << std::endl;
        // respawn pending event subprocesses
        std::shared_ptr pendingEventSubProcess = std::make_shared<StateMachine>(it->get());

        // move the triggered event subprocess to nonInterruptingEventSubProcesses
        context->nonInterruptingEventSubProcesses.push_back(std::move(*it));

        // replace iterator with pending event subprocess
        *it = pendingEventSubProcess;

//std::cerr << "After pendingEventSubProcesses: " << context->pendingEventSubProcesses.size() << std::endl;

        // prepare the new instance of the pending subprocess
        auto eventSubProcess = context->pendingEventSubProcesses.back().get();
//std::cerr << "***" << owner->parentToken << std::endl;
        eventSubProcess->run(owner->parentToken->status);
//std::cerr << "***" << std::endl;
      }
      
      // check entry scope restrictions of event-subprocess
//std::cerr << "check entry scope restrictions of event-subprocess" << std::endl;
      assert( node->parent->represents<BPMN::EventSubProcess>()->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
      auto extensionElements = node->parent->represents<BPMN::EventSubProcess>()->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if ( !extensionElements->feasibleEntry(status,*data,globals) ) {
        engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
      }
      else {
        engine->commands.emplace_back(std::bind(&Token::advanceToExiting,this), this);
      }
      return;
    }
    else if ( auto catchEvent = node->represents<BPMN::CatchEvent>();
      catchEvent &&
      node->incoming.size() == 1 &&
      node->incoming.front()->source->represents<BPMN::EventBasedGateway>()
    ) {
      engine->commands.emplace_back(std::bind(&StateMachine::handleEventBasedGatewayActivation,const_cast<StateMachine*>(owner),this), this);
    }
  }


  if ( !node || node->outgoing.empty() ) {
//std::cerr << "done: " << jsonify().dump() <<  std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), this);
    return;
  }
//std::cerr << "advanceToDeparting" << std::endl;
  advanceToDeparting();
}

void Token::advanceToExiting() {
//std::cerr << "advanceToExiting: " << jsonify().dump() << std::endl;
  auto engine = const_cast<Engine*>(owner->systemState->engine);

  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: exit timestamp at node '" + node->id + "' is larger than current time");
  }

  if ( node && node->represents<BPMN::TypedStartEvent>() ) {
    // attribute values of event subprocesses are computed when exiting typed start event  
    auto eventSubProcess = node->parent->represents<BPMN::EventSubProcess>();
    if ( !eventSubProcess ) {
      throw std::runtime_error("Token: typed start event must belong to event subprocess");
    }
    if ( auto extensionElements = owner->scope->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
      if ( !extensionElements->isInstantaneous ) {
        throw std::runtime_error("Token: Operators for event-subprocess '" + node->parent->id + "' attempt to modify timestamp");
      }
      // update status
      status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = owner->systemState->currentTime;
      extensionElements->applyOperators(status,*data,globals);

      // notify about data update
      if ( extensionElements->dataUpdate.global ) {
        owner->systemState->engine->notify( DataUpdate( extensionElements->dataUpdate.attributes ) );
      }
      else {
        owner->systemState->engine->notify( DataUpdate( owner->root->instance.value(), extensionElements->dataUpdate.attributes ) );
      }
    }
  }
  
  update(State::EXITING);

  if ( node && node->represents<BPMN::TypedStartEvent>() ) {
    // check full scope restrictions of event-subprocess
    auto eventSubProcess = node->parent->represents<BPMN::EventSubProcess>();
    assert(eventSubProcess);
//std::cerr << "check full scope restrictions of event-subprocess" << std::endl;
    if ( !eventSubProcess->extensionElements->as<BPMNOS::Model::ExtensionElements>()->fullScopeRestrictionsSatisfied(status,*data,globals) ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
    }
    else if ( node->outgoing.empty() ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), this);
    }
    else {
      advanceToDeparting();
    }
    return;
  }
  
  // check restrictions
  if ( !exitIsFeasible() ) {
    engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
    return;
  }

  auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();

  if (  extensionElements && ( extensionElements->attributes.size() || extensionElements->data.size() ) ) {
    // update global objective
    const_cast<SystemState*>(owner->systemState)->contributionsToObjective += extensionElements->getContributionToObjective(status,*data,globals);
//std::cerr << "objective updated" << std::endl;
  }
    
  if ( owned ) {
//std::cerr << "Use data of scope " << owner->scope->id << std::endl;
    data = &const_cast<StateMachine*>(owner)->data;
  }
  
  auto activity = node->represents<BPMN::Activity>();
  if ( activity && activity->loopCharacteristics.has_value() && activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
    auto& attributeRegistry = getAttributeRegistry(); 

    auto LOOP = [&]() -> bool {
      if (extensionElements->loopCondition.has_value() && extensionElements->loopCondition.value()->expression) {
        auto value = extensionElements->loopCondition.value()->expression->execute(status, *data, globals);
        assert( value.has_value() );
        if ( !value.value() ) {          
          // do not loop if loop condition is violated
          return false;
        }
      }

      if ( extensionElements->loopMaximum.has_value() && extensionElements->loopMaximum.value()->expression) {
        auto maximum = (double)extensionElements->loopMaximum.value()->expression->execute(status, *data, globals).value_or(0);
        assert( extensionElements->loopIndex.value()->expression );
        auto indexAttribute = extensionElements->loopIndex.value()->expression->isAttribute();
        assert( indexAttribute );
        auto index = attributeRegistry.getValue( indexAttribute, status, *data, globals).value();

        if ( index >= maximum ) {
          // do not loop if loop maximum loop count is reached
          return false;
        }
      }
      return true;
    }();

    if ( LOOP ) {
      awaitEntryEvent();
      return;
    }
  }

  if ( extensionElements && extensionElements->attributes.size() ) {
    // remove attributes that are no longer needed
    assert( status.size() == extensionElements->attributeRegistry.statusAttributes.size() );
    status.resize( status.size() - extensionElements->attributes.size() );
  }
  
  if ( activity && activity->loopCharacteristics.has_value() && activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard ) {
    // delegate removal of token copies for multi-instance activity to owner
    auto stateMachine = const_cast<StateMachine*>(owner);
    engine->commands.emplace_back(std::bind(&StateMachine::deleteMultiInstanceActivityToken,stateMachine,this), this);
    return;
  }

  if ( activity ) {
    auto stateMachine = const_cast<StateMachine*>(owner);
    if ( !activity->boundaryEvents.empty() ) {
      // remove tokens at boundary events
      engine->commands.emplace_back( std::bind(&StateMachine::deleteTokensAwaitingBoundaryEvent,stateMachine,this), stateMachine );
    }
  }

  // clear state machine owned by token
  if ( owned ) {
    owned->tokens.clear();
    owned->interruptingEventSubProcess.reset();
    owned->nonInterruptingEventSubProcesses.clear();
    owned->pendingEventSubProcesses.clear();
    owned.reset();  
  }

  if ( node->outgoing.empty() ) {
    engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), this);
    return;
  }
  advanceToDeparting();
}

void Token::advanceToDone() {
//std::cerr << "advanceToDone: " << jsonify().dump() << std::endl;
  update(State::DONE);

  if ( node && node->parent && node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    auto stateMachine = const_cast<StateMachine*>(owner);
    engine->commands.emplace_back(std::bind(&StateMachine::deleteAdHocSubProcessToken,stateMachine,this), this);
    return;
  }
  const_cast<StateMachine*>(owner)->attemptShutdown();
}

void Token::advanceToDeparting() {
//std::cerr << "advanceToDeparting: " /*<< jsonify().dump()*/ << std::endl;

  if ( node->outgoing.size() == 1 ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,node->outgoing.front()), this);
    return;
  }

  if ( !node->represents<BPMN::Gateway>() ) {
    throw std::runtime_error("Token: implicit split at node '" + node->id + "'");
  }

  if ( auto exclusiveGateway = node->represents<BPMN::ExclusiveGateway>() ) {
    for ( auto sequenceFlow : node->outgoing ) {
      if ( sequenceFlow != exclusiveGateway->defaultFlow ) {
        // check gatekeeper conditions
        if ( auto gatekeeper = sequenceFlow->extensionElements->as<BPMNOS::Model::Gatekeeper>() ) {
          if ( gatekeeper->conditionsSatisfied(status,*data,globals) ) {
            auto engine = const_cast<Engine*>(owner->systemState->engine);
            engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,sequenceFlow), this);
            return;
          }
        }
        else {
          throw std::logic_error("Token: no gatekeeper provided for sequence flow '" + sequenceFlow->id + "'");
        }
      }
    }

    // gatekeeper conditions are violated for all sequence flows (except default flow)
    if ( exclusiveGateway->defaultFlow ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,exclusiveGateway->defaultFlow), this);
    }
    else {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
    }
  }
  else if ( node->outgoing.size() > 1 ) {
    // non-exclusive diverging gateway
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&StateMachine::handleDivergingGateway,const_cast<StateMachine*>(owner),this), this);
  }
}

void Token::advanceToDeparted(const BPMN::SequenceFlow* sequenceFlow) {
//std::cerr << "advanceToDeparted: " << jsonify().dump() << std::endl;
  this->sequenceFlow = sequenceFlow;
  auto engine = const_cast<Engine*>(owner->systemState->engine);
  update(State::DEPARTED);
  engine->commands.emplace_back(std::bind(&Token::advanceToArrived,this), this);
}

void Token::advanceToArrived() {
//std::cerr << "advanceToArrived: " << jsonify().dump() << std::endl;
  node = sequenceFlow->target;
  update(State::ARRIVED);

  if ( node->incoming.size() > 1 && !node->represents<BPMN::ExclusiveGateway>() ) {
    if ( !node->represents<BPMN::Gateway>() ) {
      throw std::runtime_error("Token: implicit join at node '" + node->id + "'");
    }
    update(State::WAITING);

    awaitGatewayActivation();

    const_cast<StateMachine*>(owner)->attemptGatewayActivation(node);

    return;
  }

  if ( node->represents<BPMN::Activity>() ) {
    awaitReadyEvent();
  }
  else {
    sequenceFlow = nullptr;
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToEntered,this), this);
  }
}

void Token::advanceToFailed() {
//std::cerr << " advanceToFailed: " << jsonify().dump() << std::endl;
  auto engine = const_cast<Engine*>(owner->systemState->engine);
 
  if ( owned ) {
//std::cerr << "Use data of scope " << owner->scope->id << std::endl;
    data = &const_cast<StateMachine*>(owner)->data;

    if ( !owned->scope ) {
      owned.reset(); 
    }
    else if ( auto activity = owned->scope->represents<BPMN::Activity>();
      activity && activity->isForCompensation
    ) {
      owned.reset(); 
    }
    else if ( owned->tokens.size() || owned->interruptingEventSubProcess ) {
    // owned state machine may require compensation
//std::cerr << "Failing " << owned->scope->id << std::endl;
      update(State::FAILING);
      engine->commands.emplace_back(std::bind(&Token::terminate,this), this);
      return;
    }
  }
  update(State::FAILED);
  engine->commands.emplace_back(std::bind(&StateMachine::handleFailure,const_cast<StateMachine*>(owner),this), this);
}

void Token::terminate() {
//std::cerr << "terminate " << (node ? node->id : owner->scope->id ) << std::endl;
  auto engine = const_cast<Engine*>(owner->systemState->engine);

  assert(owned);

  owned->clearObsoleteTokens();
  
  if ( owned->compensationTokens.size() ) {
    // don't delete state machine and wait for all compensations to be completed
    // before continuing with termination
    owned->compensate(owned->compensationTokens, this);
    return;
  }

  // delete state machine
  owned.reset();
  
  // all compensations have been completed, now handle failure
  engine->commands.emplace_back(std::bind(&Token::update,this,Token::State::FAILED), this);
  engine->commands.emplace_back(std::bind(&StateMachine::handleFailure,const_cast<StateMachine*>(owner),this), this);
//std::cerr << "terminated" << std::endl;
}

void Token::awaitCompensation() {
  auto activity = node->as<BPMN::Activity>();
  if ( auto compensationActivity = activity->compensatedBy->represents<BPMN::Activity>();
    compensationActivity &&
    activity->loopCharacteristics != compensationActivity->loopCharacteristics
  ) {
    throw std::runtime_error("Token: compensation activities must have the same loop characteristics as the compensated activity '" + node->id + "'");
  }

  auto stateMachine = const_cast<StateMachine*>(owner);
  auto engine = const_cast<Engine*>(owner->systemState->engine);

  if ( auto subProcess = node->represents<BPMN::SubProcess>();
    subProcess && subProcess->compensationEventSubProcess
  ) {
    // create compensation event subprocess with token
    engine->commands.emplace_back( std::bind(&StateMachine::createCompensationEventSubProcess,stateMachine,subProcess->compensationEventSubProcess, status), stateMachine );
  }
  else {
    // find compensate boundary event
    auto it = std::find_if(activity->boundaryEvents.begin(), activity->boundaryEvents.end(), [](BPMN::FlowNode* boundaryEvent) {
      return ( boundaryEvent->represents<BPMN::CompensateBoundaryEvent>() );
    });
    if ( it != activity->boundaryEvents.end() ) {
      // create compensation token 
      engine->commands.emplace_back( std::bind(&StateMachine::createCompensationTokenForBoundaryEvent,stateMachine,*it, status), stateMachine );
    }
  }
}

void Token::awaitReadyEvent() {
//std::cerr << "awaitReadyEvent" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingReadyEvent.emplace_back(weak_from_this());
}

void Token::awaitEntryEvent() {
//std::cerr << "awaitEntryEvent" << std::endl;

  auto systemState = const_cast<SystemState*>(owner->systemState);
  if ( node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto tokenAtSequentialPerformer = getSequentialPerformerToken();
//std::cerr << "Token: " << tokenAtSequentialPerformer->jsonify() << "  pendingSequentialEntries add " << jsonify() << std::endl;
    tokenAtSequentialPerformer->pendingSequentialEntries.emplace_back(weak_from_this());
    
    if ( tokenAtSequentialPerformer->performing ) {
//std::cerr << "Token: Waiting for perfomer to become idle" << std::endl;
      // defer decision request until performer becomes idle again
      return;
    }
  }
  
  decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::EntryRequest );
  systemState->pendingEntryDecisions.emplace_back( weak_from_this(), decisionRequest );
//std::cerr << "Token: Waiting for entry" << std::endl;
  owner->systemState->engine->notify(decisionRequest.get());
}

void Token::awaitChoiceEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::ChoiceRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingChoiceDecisions.emplace_back( weak_from_this(), decisionRequest );

}

void Token::awaitTaskCompletionEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto time = status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
  systemState->tokensAwaitingCompletionEvent.emplace(time,weak_from_this());
}

void Token::awaitExitEvent() {
//std::cerr << "awaitExitEvent" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::ExitRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingExitDecisions.emplace_back( weak_from_this(), decisionRequest );

}

void Token::awaitMessageDelivery() {
//std::cerr << "awaitMessageDelivery" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::MessageDeliveryRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingMessageDeliveryDecisions.emplace_back( weak_from_this(), decisionRequest );
}

void Token::awaitTimer(BPMNOS::number time) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingTimer.emplace(time,weak_from_this());
}

void Token::awaitSignal(BPMNOS::number name) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingSignal[name].emplace_back(weak_from_this());
}

void Token::awaitConditions(BPMNOS::number instanceId) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingCondition[instanceId].emplace_back(weak_from_this());
}

void Token::awaitGatewayActivation() {
//std::cerr << "awaitGatewayActivation" << std::endl;

  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto stateMachine = const_cast<StateMachine*>(owner);
  auto gatewayIt = systemState->tokensAwaitingGatewayActivation[stateMachine].find(node);
  if (gatewayIt == systemState->tokensAwaitingGatewayActivation[stateMachine].end()) {
    // The key is not found, so insert a new entry and get an iterator to it.
    gatewayIt = systemState->tokensAwaitingGatewayActivation[stateMachine].insert({node,{}}).first;
  }

  auto& [key,tokens] = *gatewayIt;
  tokens.emplace_back(this);
}

template<typename DecisionType, typename... Args>
std::shared_ptr<DecisionType> Token::createDecisionRequest(Args&&... args) {
//std::cerr << "Create pending decision for token in state " << stateName[(int)state] << " at node " << node->id << std::endl;
  auto event = std::make_shared<DecisionType>(this, std::forward<Args>(args)...);
  owner->systemState->engine->notify(event.get());
  return event;
}


void Token::withdraw() {
  if ( state != State::DONE && state != State::FAILED 
  ) {
    update(State::WITHDRAWN);
  }
}

void Token::emitSignal() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  assert( node->extensionElements->represents<BPMNOS::Model::Signal>() );
  auto signalDefinition = node->extensionElements->as<BPMNOS::Model::Signal>();

  auto& waitingTokens = systemState->tokensAwaitingSignal[signalDefinition->name];
  if ( !waitingTokens.empty() ) {
    // determine signal content
    VariedValueMap contentValueMap = getSignalContent(signalDefinition->contentMap);

    for ( auto& [token_ptr] : waitingTokens ) {
      auto token = token_ptr.lock();
      assert( token );
      // receive signal content
      token->setSignalContent(contentValueMap);
      
      // advance receiving token
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
    }
    waitingTokens.clear();
  }
}

BPMNOS::VariedValueMap Token::getSignalContent(const BPMNOS::Model::ContentMap& contentMap) {
  auto& attributeRegistry = getAttributeRegistry();
  VariedValueMap contentValueMap;
  for (auto& [key,contentDefinition] : contentMap) {
    if ( status[contentDefinition->attribute->index].has_value() ) {
      contentValueMap.emplace( key, attributeRegistry.getValue(contentDefinition->attribute,status,*data,globals) );
    }
    else {
      contentValueMap.emplace( key, std::nullopt );
    }
  }
  return contentValueMap;
}

void Token::setSignalContent(BPMNOS::VariedValueMap& sourceMap) {
  auto& attributeRegistry = getAttributeRegistry();
  assert( node->extensionElements->represents<BPMNOS::Model::Signal>() );
  auto signalDefinition = node->extensionElements->as<BPMNOS::Model::Signal>();

  size_t counter = 0;
  for (auto& [key,contentValue] : sourceMap) {
    if ( auto it = signalDefinition->contentMap.find(key); it != signalDefinition->contentMap.end() ) {
      auto& [_,definition] = *it;
      auto attribute = definition->attribute;
//std::cerr << "Attribute: " << attribute.name << "/" << attribute.index << std::endl;
      if ( std::holds_alternative< std::optional<number> >(contentValue) && std::get< std::optional<number> >(contentValue).has_value() ) {
        // use attribute value of signal
        attributeRegistry.setValue(attribute, status, *data, globals, std::get< std::optional<number> >(contentValue).value() );
      }
      else if (std::holds_alternative<std::string>(contentValue)) {
        // use default value of emitter
        Value value = std::get< std::string >(contentValue);
        attributeRegistry.setValue(attribute, status, *data, globals, BPMNOS::to_number(value,attribute->type) );
      }
      else {
        attributeRegistry.setValue(attribute, status, *data, globals, std::nullopt );
      }
    }
    else {
      // key in signal content, but not in recipient content
      counter++;
    }
  }

  if ( signalDefinition->contentMap.size() > sourceMap.size() - counter ) {
    // recipient has keys in content that are not in signal content
    for (auto& [key,definition] : signalDefinition->contentMap) {
      if ( !sourceMap.contains(key) ) {
        // key in recipient content, but not in message content
        attributeRegistry.setValue(definition->attribute, status, *data, globals, std::nullopt );
      }
    }
  }

  // notify about data update
  owner->systemState->engine->notify( DataUpdate( owner->root->instance.value(), signalDefinition->updatedData ) );
}


void Token::sendMessage(size_t index) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->messages.emplace_back(std::make_shared<Message>(this,index));
  auto& message = systemState->messages.back();

  if ( message->recipient.has_value() ) {
    // TODO: add to unsent or outbox
    auto it = systemState->archive.find((long unsigned int)message->recipient.value());
    if ( it == systemState->archive.end() ) {
//std::cerr << "Message unsent" << std::endl;
      // defer sending of message to when recipient is instantiated
      systemState->unsent[(long unsigned int)message->recipient.value()].emplace_back(message->weak_from_this());
    }
    else if ( auto stateMachine = it->second.lock() ) {
//std::cerr << "Message sent from " << node->id << std::endl;
      systemState->inbox[stateMachine.get()].emplace_back(message->weak_from_this());
      systemState->outbox[node].emplace_back(message->weak_from_this());
    }
  }
  else {
//std::cerr << "Message sent from " << node->id << std::endl;
    systemState->outbox[node].emplace_back(message->weak_from_this());
  }

  if ( node->represents<BPMN::SendTask>() ) {
    systemState->messageAwaitingDelivery[this] = message->weak_from_this();
  }
  
  // message->state = Message::State::CREATED;
  owner->systemState->engine->notify(message.get());

} 

Token* Token::getSequentialPerformerToken() const {
  auto adHocSubProcess = node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
  assert( adHocSubProcess );

  Token* sequentialPerfomerToken = owner->parentToken;
  while (sequentialPerfomerToken->node && sequentialPerfomerToken->node != adHocSubProcess->performer) {
    sequentialPerfomerToken = sequentialPerfomerToken->owner->parentToken;
  }
  return sequentialPerfomerToken;
}

void Token::occupySequentialPerformer() {
  auto tokenAtSequentialPerformer = getSequentialPerformerToken();
//std::cerr << "Token::releaseSequentialPerformer " << std::endl;
  assert( !tokenAtSequentialPerformer->performing );
  tokenAtSequentialPerformer->pendingSequentialEntries.remove(this);

  // sequential performer becomes busy
  tokenAtSequentialPerformer->performing = this;
  owner->systemState->engine->notify(SequentialPerformerUpdate(tokenAtSequentialPerformer));
  
  for ( auto& [ token_ptr ]  : tokenAtSequentialPerformer->pendingSequentialEntries ) {
    if ( auto activityToken = token_ptr.lock() ) {
      // withdraw entry decision requests for each child activity awaiting entry
      assert( activityToken->node->represents<BPMN::Activity>() && activityToken->state == State::READY );
      assert( activityToken->decisionRequest );
//std::cerr << "Token: Withdraw decision " << activityToken->jsonify() << std::endl;
      activityToken->decisionRequest.reset();
    }
  }
}

void Token::releaseSequentialPerformer() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto tokenAtSequentialPerformer = getSequentialPerformerToken();
//std::cerr << "Token::releaseSequentialPerformer " << std::endl;
  assert( tokenAtSequentialPerformer->performing );
  
  // sequential performer becomes idle
  tokenAtSequentialPerformer->performing = nullptr;
  systemState->engine->notify(SequentialPerformerUpdate(tokenAtSequentialPerformer));

  for ( auto& [ token_ptr ]  : tokenAtSequentialPerformer->pendingSequentialEntries ) {
    if ( auto activityToken = token_ptr.lock() ) {
      // create entry decision requests for each child activity awaiting entry
      assert( activityToken->node->represents<BPMN::Activity>() && activityToken->state == State::READY );
      assert( !activityToken->decisionRequest );
//std::cerr << "Token: Renew decision request" << activityToken->jsonify() << std::endl;
      activityToken->decisionRequest = std::make_shared<DecisionRequest>( activityToken.get(), Observable::Type::EntryRequest );
      systemState->pendingEntryDecisions.emplace_back(activityToken,activityToken->decisionRequest);
      owner->systemState->engine->notify(activityToken->decisionRequest.get());
    }
  }
}


void Token::update(State newState) {
  assert( status.size() >= 1 );
  assert( data->size() >= 1 );
  assert( (*data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().has_value() );
  assert( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() );

  state = newState;
  auto now = owner->systemState->getTime();

//std::cerr << "update at time " << now << ": " << jsonify().dump() << std::endl;
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
//std::cerr << "Set timestamp to " << now << std::endl;
    // increase timestamp if necessary
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = now;
  }
  else if (
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > now
    && state != State::WITHDRAWN
    && !(state == State::BUSY && node->represents<BPMN::Task>() && !node->represents<BPMN::ReceiveTask>() && !node->represents<BPMNOS::Model::DecisionTask>())
  ) {
//std::cerr << now << "/" << this->jsonify().dump() << now << std::endl;
    throw std::runtime_error("Token: timestamp at node '" + node->id + "' is larger than current time");
  }
  notify();
}

void Token::notify() const {
  owner->systemState->engine->notify(this);
}
