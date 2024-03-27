#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "DecisionRequest.h"
#include "SequentialPerformerUpdate.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Gatekeeper.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/utility/src/erase.h"
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

      if ( state == State::READY && activity->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
        systemState->tokenAtSequentialPerformer.erase(this);
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

const BPMNOS::Model::AttributeMap& Token::getStatusAttributes() const {
  if ( !node ) {
    return owner->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->statusAttributes;
  }

  if ( auto extensionElements = node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>() ) {
    return extensionElements->statusAttributes;
  }

  // return status attributes of parent for nodes without extension elements
  if ( !owner->parentToken ) {
    throw std::runtime_error("Token: cannot determine attribute map");
  }

  return owner->parentToken->getStatusAttributes();
}

const BPMNOS::Model::AttributeMap& Token::getDataAttributes() const {
  return owner->scope->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->dataAttributes;
}

void Token::setStatus(const BPMNOS::Values& other) {
  assert( (int)BPMNOS::Model::ExtensionElements::Index::Instance == 0 );
  size_t size = ( other.size() >= status.size() ? status.size() : other.size() );
  std::copy(other.begin() + 1, other.begin() + (std::ptrdiff_t)size , status.begin()+1);
}

nlohmann::ordered_json Token::jsonify() const {
  nlohmann::ordered_json jsonObject;
  jsonObject["processId"] = owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string(status[BPMNOS::Model::ExtensionElements::Index::Instance].value(),STRING);
  if ( node ) {
    jsonObject["nodeId"] = node->id;
  }
  if ( sequenceFlow ) {
    jsonObject["sequenceFlowId"] = sequenceFlow->id;
  }
  jsonObject["state"] = stateName[(int)state];
  jsonObject["status"] = nlohmann::ordered_json::object();

  auto& statusAttributes = getStatusAttributes();
  for (auto& [attributeName,attribute] : statusAttributes ) {
    if ( attribute->index >= status.size() ) {
      // skip attribute that is not in status
      continue;
    }

    if ( !status[attribute->index].has_value() ) {
      jsonObject["status"][attributeName] = nullptr ;
    }
    else if ( attribute->type == BOOLEAN) {
      bool value = (bool)status[attribute->index].value();
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == INTEGER) {
      int value = (int)status[attribute->index].value();
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == DECIMAL) {
      double value = (double)status[attribute->index].value();
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(status[attribute->index].value(),attribute->type);
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == COLLECTION) {
      std::string value = BPMNOS::to_string(status[attribute->index].value(),attribute->type);
      jsonObject["status"][attributeName] = value ;
    }
  }

  if ( owner->data.empty() ) {
    return jsonObject;
  }

  jsonObject["data"] = nlohmann::ordered_json::object();

  auto& dataAttributes = getDataAttributes();
  for (auto& [attributeName,attribute] : dataAttributes ) {
    if ( attribute->index >= owner->data.size() ) {
      // skip attribute that is not in owner->data
    }

    std::optional<BPMNOS::number>& dataValue = owner->data[attribute->index];
    if ( !dataValue.has_value() ) {
      jsonObject["data"][attributeName] = nullptr ;
    }
    else if ( attribute->type == BOOLEAN) {
      bool value = (bool)dataValue.value();
      jsonObject["data"][attributeName] = value ;
    }
    else if ( attribute->type == INTEGER) {
      int value = (int)dataValue.value();
      jsonObject["data"][attributeName] = value ;
    }
    else if ( attribute->type == DECIMAL) {
      double value = (double)dataValue.value();
      jsonObject["data"][attributeName] = value ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(dataValue.value(),attribute->type);
      jsonObject["data"][attributeName] = value ;
    }
    else if ( attribute->type == COLLECTION) {
      std::string value = BPMNOS::to_string(dataValue.value(),attribute->type);
      jsonObject["data"][attributeName] = value ;
    }
  }

  return jsonObject;
}


bool Token::entryIsFeasible() const {
  if ( !node ) {
//std::cerr << stateName[(int)state] << "/" << owner->scope->id <<std::endl;
    assert( owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    return owner->process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleEntry(status);
  }

  assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  return node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleEntry(status);
}

bool Token::exitIsFeasible() const {
  if ( !node ) {
//std::cerr << stateName[(int)state] << "/" << owner->scope->id <<std::endl;
    assert( owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    return owner->process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleExit(status);
  }

  assert( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  return node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->feasibleExit(status);
}

/*
bool Token::satisfiesInheritedRestrictions() const {
//std::cerr << "satisfiesInheritedRestrictions?" << std::endl;
  // check restrictions within ancestor scopes
  const BPMN::Node* ancestor = node->parent;
  while ( ancestor ) {
    assert( ancestor->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    if ( !ancestor->extensionElements->as<BPMNOS::Model::ExtensionElements>()->fullScopeRestrictionsSatisfied(status) ) {
      return false;
    }
    if ( auto eventSubProcess = ancestor->represents<BPMN::EventSubProcess>();
      eventSubProcess && 
      ( eventSubProcess->startEvent->represents<BPMN::ErrorStartEvent>() || eventSubProcess->startEvent->represents<BPMN::CompensateStartEvent>() )
    ) {
      // error and compensate event subprocesses do not inherit restrictions
      break;
    }
    else if ( auto activity = ancestor->represents<BPMN::Activity>();
      activity && activity->isForCompensation
    ) {
      // compensation activities do not inherit restrictions
      break;
    }
    else if ( auto child = ancestor->represents<BPMN::ChildNode>() ) {
      ancestor = child->parent;
    }
    else {
      break;
    }
  }
  return true;
}
*/

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

  update(State::READY);
  
  if ( auto activity = node->represents<BPMN::Activity>();
    activity && 
    activity->loopCharacteristics.has_value()
  ) {
    if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
      throw std::runtime_error("Token: standard loop marker at activity '" + node->id + "' is not yet supported");
    }
    else {
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
    // process operators are applied upon entry
    if ( auto extensionElements = owner->process->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
      if ( !extensionElements->isInstantaneous ) {
        throw std::runtime_error("StateMachine: Operators for process '" + node->id + "' attempt to modify timestamp");
      }
      // update status
      extensionElements->applyOperators(status);
    }
  }
  else if ( node->represents<BPMN::SubProcess>() || 
    node->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
  ) {
//std::cerr << "node->represents<BPMN::SubProcess>()" << std::endl;
    // subprocess operators are applied upon entry
    if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
      if ( !extensionElements->isInstantaneous ) {
        throw std::runtime_error("StateMachine: Operators for subprocess '" + node->id + "' attempt to modify timestamp");
      }
      // update status
      extensionElements->applyOperators(status);
    }
  }

  update(State::ENTERED);
//std::cerr << "updatedToEntered" << std::endl;

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

//std::cerr << "check restrictions: " << isFeasible() << std::endl;
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
    else if ( node->represents<BPMN::MessageThrowEvent>() ) {
      assert( !node->represents<BPMN::SendTask>() );
      sendMessage();
    }
    else if ( auto compensateThrowEvent = node->represents<BPMN::CompensateThrowEvent>() ) {
      auto context = const_cast<StateMachine*>(owner->parentToken->owned.get());

      if ( auto eventSubProcess = owner->scope->represents<BPMN::EventSubProcess>();
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
    // TODO: determine time
    auto trigger = node->extensionElements->as<BPMNOS::Model::Timer>()->trigger.get();
    BPMNOS::number time;

    if ( trigger && trigger->attribute && status[trigger->attribute.value().get().index].has_value() ) {
      time = status[trigger->attribute.value().get().index].value();
    }
    else if ( trigger && trigger->value && trigger->value.has_value() ) {
      time = (int)trigger->value.value().get();
    }
    else {
      throw std::runtime_error("Token: no trigger given for node '" + node->id + "'");
    }

    if ( time > owner->systemState->getTime() ) {
      awaitTimer(time);
    }
    else {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);
    }
  }
  else if ( node->represents<BPMN::MessageCatchEvent>() ) {
    if ( auto receiveTask = node->represents<BPMN::ReceiveTask>();
      receiveTask && 
      receiveTask->loopCharacteristics.has_value()
    ) {
      throw std::runtime_error("Token: receive tasks with loop characteristics are not yet supported");
    }
    else {
      awaitMessageDelivery();
    }
  }
  else if ( node->represents<BPMN::Task>() ) {
    if ( node->represents<BPMNOS::Model::DecisionTask>() ) {
      awaitChoiceEvent();
    }
    else {
      // apply operators for completion status
      if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
        if ( extensionElements->isInstantaneous ) {
          // update status directly
          extensionElements->applyOperators(status);

          if ( auto sendTask = node->represents<BPMN::SendTask>() ) {
            if ( sendTask->loopCharacteristics.has_value() ) {
              // multi-instance send task
              if ( !extensionElements->loopIndex.has_value() || !extensionElements->loopIndex->get()->attribute.has_value() ) {
                throw std::runtime_error("Token: send tasks with loop characteristics requires attribute holding loop index");
              }
              size_t attributeIndex = extensionElements->loopIndex->get()->attribute.value().get().index;
              if ( !status[attributeIndex].has_value() ) { 
                throw std::runtime_error("Token: cannot find loop index for send tasks with loop characteristics");
              }
              sendMessage( (size_t)(int)status[attributeIndex].value() );
            }
            else {
              sendMessage();
            }
            // wait for delivery
            return;
          }
        }
        else {
          if ( node->represents<BPMN::SendTask>() ) {
            throw std::runtime_error("Token: send tasks must not have operators modifying timestamp ");
          }
          // defer status update status until task completion
          awaitTaskCompletionEvent();
          return;
        }
      }

      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), this);

    }
  }
}

/*
void Token::advanceToCompleted(const Values& statusUpdate) {
  status = statusUpdate;
  advanceToCompleted();
}
*/

void Token::advanceToCompleted() {
//std::cerr << "advanceToCompleted: " << jsonify().dump() << std::endl;

  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    if ( node ) {
      throw std::runtime_error("Token: completion timestamp at node '" + node->id + "' is larger than current time");
    }
    else {
      throw std::runtime_error("Token: completion timestamp for process '" + owner->process->id + "' is larger than current time");
    }
  }

  update(State::COMPLETED);

  auto engine = const_cast<Engine*>(owner->systemState->engine);

  if ( !node ) {
    // update global objective
    assert( owner->scope->extensionElements->as<BPMNOS::Model::ExtensionElements>() );
    const_cast<SystemState*>(owner->systemState)->objective += owner->scope->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getContributionToObjective(status);

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
        const_cast<SystemState*>(owner->systemState)->objective += node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getContributionToObjective(status);
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
      auto eventSubProcess = owner->scope->represents<BPMN::EventSubProcess>();
      if ( !eventSubProcess ) {
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

      // initiate nested event subprocesses when event subprocess is triggered
//      engine->commands.emplace_back(std::bind(&StateMachine::initiateEventSubprocesses,it->get(),this), this);

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
    }
    else if ( auto catchEvent = node->represents<BPMN::CatchEvent>();
      catchEvent &&
      node->incoming.size() == 1 &&
      node->incoming.front()->source->represents<BPMN::EventBasedGateway>()
    ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
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

  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: exit timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::EXITING);

  auto engine = const_cast<Engine*>(owner->systemState->engine);

  // check restrictions
  if ( !exitIsFeasible() ) {
    engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), this);
    return;
  }

  if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
       extensionElements && extensionElements->attributes.size()
  ) {
    // update global objective
    const_cast<SystemState*>(owner->systemState)->objective += extensionElements->getContributionToObjective(status);

    // remove attributes that are no longer needed
    status.resize( extensionElements->statusAttributes.size() - extensionElements->attributes.size() );
  }

  if ( auto activity = node->represents<BPMN::Activity>();
    activity && 
    activity->loopCharacteristics.has_value()
  ) {
    if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::Standard ) {
      throw std::runtime_error("Token: standard loop marker at activity '" + node->id + "' is not yet supported");
    }
    else {
      awaitCompensation();

      // delegate removal of token copies for multi-instance activity to owner
      auto stateMachine = const_cast<StateMachine*>(owner);
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::deleteMultiInstanceActivityToken,stateMachine,this), this);
      return;
    }
  }

    if ( auto activity = node->represents<BPMN::Activity>() ) {
    auto stateMachine = const_cast<StateMachine*>(owner);
    if ( !activity->boundaryEvents.empty() ) {
      // remove tokens at boundary events
      engine->commands.emplace_back( std::bind(&StateMachine::deleteTokensAwaitingBoundaryEvent,stateMachine,this), stateMachine );

      awaitCompensation();
    }

    if ( auto subProcess = node->represents<BPMN::SubProcess>();
      subProcess && subProcess->compensationEventSubProcess
    ) {
      auto stateMachine = const_cast<StateMachine*>(owner);
      // create compensation event subprocess
      engine->commands.emplace_back( std::bind(&StateMachine::createCompensationEventSubProcess,stateMachine,subProcess->compensationEventSubProcess, status), stateMachine );
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
//std::cerr << "advanceToDeparting: " << jsonify().dump() << std::endl;

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
          if ( gatekeeper->restrictionsSatisfied(status) ) {
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
//std::cerr << this << " / advanceToArrived: " << jsonify().dump() << std::endl;
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
    if ( auto activity = owned->scope->represents<BPMN::Activity>();
      activity && activity->isForCompensation
    ) {
      owned.reset(); 
    }
    else if ( owned->tokens.size() || owned->interruptingEventSubProcess ) {
    // owned state machine may require compensation
//std::cerr << "Failing " << owned->scope->id << std::endl;
      update(State::FAILING);
      engine->commands.emplace_back(std::bind(&Token::terminate,this), this);
//      engine->commands.emplace_back(std::bind(&StateMachine::terminate,owned.get()), owned.get());
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

/*
  // remove state machine from parent
  if ( auto eventSubProcess = owner->scope->represents<BPMN::EventSubProcess>();
    eventSubProcess && eventSubProcess->startEvent->isInterrupting
  ) {
    const_cast<StateMachine*>(owner)->interruptingEventSubProcess.reset();
  }
*/
  // delete state machine
  owned.reset();
  
  // all compensations have been completed, now handle failure
  engine->commands.emplace_back(std::bind(&Token::update,this,Token::State::FAILED), this);
  engine->commands.emplace_back(std::bind(&StateMachine::handleFailure,const_cast<StateMachine*>(owner),this), this);
//std::cerr << "terminated" << std::endl;
}

void Token::awaitCompensation() {
  auto activity = node->as<BPMN::Activity>();
  if ( activity->compensatedBy ) {
    if ( auto compensationActivity = activity->compensatedBy->represents<BPMN::Activity>();
      compensationActivity &&
      activity->loopCharacteristics != compensationActivity->loopCharacteristics
    ) {
      throw std::runtime_error("Token: compensation activities with different loop characteristics as the compensated activity '" + node->id + "' are not yet supported");
    }

    auto stateMachine = const_cast<StateMachine*>(owner);
    auto engine = const_cast<Engine*>(owner->systemState->engine);

    // find compensate boundary event
    auto it = std::find_if(activity->boundaryEvents.begin(), activity->boundaryEvents.end(), [](BPMN::FlowNode* boundaryEvent) {
      return ( boundaryEvent->represents<BPMN::CompensateBoundaryEvent>() );
    });
    if ( it != activity->boundaryEvents.end() ) {
      // create compensation token allowing
      engine->commands.emplace_back( std::bind(&StateMachine::createCompensationTokenForBoundaryEvent,stateMachine,*it, status), stateMachine );
    }
  }
}

void Token::awaitReadyEvent() {
//std::cerr << "awaitReadyEvent" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingReadyEvent.emplace_back(weak_from_this());
/*
  auto event = createPendingEvent<ReadyEvent>();
  systemState->pendingReadyEvents.emplace_back( weak_from_this(), event );
*/
}

void Token::awaitEntryEvent() {
//std::cerr << "awaitEntryEvent" << std::endl;

  auto systemState = const_cast<SystemState*>(owner->systemState);
  if ( node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    systemState->tokenAtSequentialPerformer[this] = getSequentialPerfomerToken();
  }
  
/*
  auto event = createDecisionRequest<EntryDecision>();
  systemState->pendingEntryDecisions.emplace_back( weak_from_this(), event );
*/
  auto decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::EntryRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingEntryEvents.emplace_back( weak_from_this(), decisionRequest );

}

void Token::awaitChoiceEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
/*
  auto event = createDecisionRequest<ChoiceDecision>();
  systemState->pendingChoiceDecisions.emplace_back( weak_from_this(), event );
*/
  auto decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::ChoiceRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingChoiceEvents.emplace_back( weak_from_this(), decisionRequest );

}

void Token::awaitTaskCompletionEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  if ( auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
       extensionElements && extensionElements->operators.size()
  ) {
    extensionElements->applyOperators(status);
    if ( !status[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() ) {
      throw std::runtime_error("Token: timestamp at node '" + node->id + "' is deleted");
    }
  }
  auto time = status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();

  systemState->tokensAwaitingCompletionEvent.emplace(time,weak_from_this());
/*
  auto event = createPendingEvent<CompletionEvent>();
  systemState->pendingCompletionEvents.emplace( time, weak_from_this(), event );
*/
}

void Token::awaitExitEvent() {
//std::cerr << "awaitExitEvent" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
/*
  auto event = createDecisionRequest<ExitDecision>();
  systemState->pendingExitDecisions.emplace_back( weak_from_this(), event );
*/
  auto decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::ExitRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingExitEvents.emplace_back( weak_from_this(), decisionRequest );

}

void Token::awaitMessageDelivery() {
//std::cerr << "awaitMessageDelivery" << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
/*
  auto recipientHeader = node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageDefinitions.front()->getRecipientHeader(status);
  auto event = createDecisionRequest<MessageDeliveryDecision>(recipientHeader);
  systemState->pendingMessageDeliveryDecisions.emplace_back( weak_from_this(), event );
*/

  auto decisionRequest = std::make_shared<DecisionRequest>( this, Observable::Type::MessageDeliveryRequest );
  owner->systemState->engine->notify(decisionRequest.get());
  systemState->pendingMessageDeliveryEvents.emplace_back( weak_from_this(), decisionRequest );
}

void Token::awaitTimer(BPMNOS::number time) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingTimer.emplace(time,weak_from_this());
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

void Token::sendMessage(size_t index) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->messages.emplace_back(std::make_shared<Message>(this,index));
  auto& message = systemState->messages.back();

  if ( message->recipient.has_value() ) {
    // TODO: add to unsent or outbox
    auto it = systemState->archive.find(message->recipient.value());
    if ( it == systemState->archive.end() ) {
//std::cerr << "Message unsent" << std::endl;
      // defer sending of message to when recipient is instantiated
      systemState->unsent[message->recipient.value()].emplace_back(message->weak_from_this());
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

Token* Token::getSequentialPerfomerToken() const {
  auto activity = node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>();
  assert( activity );

  Token* sequentialPerfomerToken = owner->parentToken;
  while (sequentialPerfomerToken->node && sequentialPerfomerToken->node != activity->performer) {
    sequentialPerfomerToken = sequentialPerfomerToken->owner->parentToken;
  }
  return sequentialPerfomerToken;
}

void Token::update(State newState) {
  assert( status.size() >= 2 );
  assert( status[BPMNOS::Model::ExtensionElements::Index::Instance].has_value() );
  assert( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() );

  state = newState;
  auto now = owner->systemState->getTime();

//std::cerr << "update at time " << now << ": " << jsonify().dump() << std::endl;
  if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value() < now ) {
//std::cerr << "Set timestamp to " << now << std::endl;
    // increase timestamp if necessary
    status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = now;
  }
  else if ( status[BPMNOS::Model::ExtensionElements::Index::Timestamp] > now
    && state != State::WITHDRAWN
  ) {
//std::cerr << now << "/" << this->jsonify().dump() << now << std::endl;
    throw std::runtime_error("Token: timestamp at node '" + node->id + "' is larger than current time");
  }
  notify();
}

void Token::notify() const {
  owner->systemState->engine->notify(this);
}
