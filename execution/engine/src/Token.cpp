#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/extensionElements/Gatekeeper.h"
#include "model/parser/src/extensionElements/Timer.h"
#include "model/parser/src/JobShop.h"
#include "model/parser/src/ResourceActivity.h"
#include "model/parser/src/DecisionTask.h"
#include "execution/listener/src/Listener.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

Token::Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status)
  : owner(owner)
  , node(node)
  , sequenceFlow(nullptr)
  , state(State::CREATED)
  , status(status)
{
}

Token::Token(const Token* other) 
  : owner(other->owner)
  , node(other->node)
  , sequenceFlow(nullptr)
  , state(other->state)
  , status(other->status)
{
}

Token::Token(const std::vector<Token*>& others)
  : owner(others.front()->owner)
  , node(others.front()->node)
  , sequenceFlow(nullptr)
  , state(others.front()->state)
  , status(others.front()->status)
{
  for ( auto other : others | std::views::drop(1) ) {
    mergeStatus(other);
  }
}

const BPMNOS::Model::AttributeMap& Token::getAttributeMap() const {
  if ( !node ) {
    return owner->process->extensionElements->as<const Model::Status>()->attributeMap;
  }

  if ( auto status = node->extensionElements->represents<const Model::Status>(); status ) {
    return status->attributeMap;
  }

  if ( !owner->parentToken ) {
    throw std::runtime_error("Token: cannot determine attribute map");    
  }

  return owner->parentToken->getAttributeMap();
}

nlohmann::ordered_json Token::jsonify() const {
  nlohmann::ordered_json jsonObject;
  jsonObject["processId"] = owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string(status[Model::Status::Index::Instance].value(),STRING);
  if ( node ) {
    jsonObject["nodeId"] = node->id;
  }
  if ( sequenceFlow ) {
    jsonObject["sequenceFlowId"] = sequenceFlow->id;
  }
  jsonObject["state"] = stateName[(int)state];
  jsonObject["status"] = nlohmann::json::object();

  auto& attributeMap = getAttributeMap();
  for (auto& [attributeName,attribute] : attributeMap ) {
    if ( !status[attribute->index].has_value() ) {
      jsonObject["status"][attributeName] = nullptr ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(status[attribute->index].value(),STRING);
      jsonObject["status"][attributeName] = value ;
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
  }

  return jsonObject;
}

bool Token::isFeasible() {
  // TODO check restrictions within current scope and ancestor scopes

  return true;
}

/*
void Token::advanceFromCreated() {
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
*/

void Token::advanceToReady() {
//std::cerr << "advanceToReady" << std::endl;

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: ready timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::READY);

//std::cerr << "->awaitEntryEvent" << std::endl;
  awaitEntryEvent();
}

void Token::advanceToEntered() {
//std::cerr << "advanceToEntered" << std::endl;
  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    if ( node ) {
      throw std::runtime_error("Token: entry timestamp at node '" + node->id + "' is larger than current time");
    }
    else {
      throw std::runtime_error("Token: entry timestamp for process '" + owner->process->id + "' is larger than current time");
    }
  }

  if ( !node ) {
    // process operators are applied upon entry
    if ( auto statusExtension = owner->process->extensionElements->as<BPMNOS::Model::Status>(); 
         statusExtension
    ) {
      for ( auto& operator_ : statusExtension->operators ) {
        if ( operator_->attribute->index == BPMNOS::Model::Status::Index::Timestamp ) {
          throw std::runtime_error("StateMachine: Operator '" + operator_->id + "' for process '" + owner->process->id + "' attempts to modify timestamp");
        }
        operator_->apply(status);
      }
    }
  }
  else if ( node->represents<BPMN::SubProcess>() ) {
    // subprocess operators are applied upon entry
    if ( auto statusExtension = node->extensionElements->as<BPMNOS::Model::Status>(); 
         statusExtension
    ) {
      for ( auto& operator_ : statusExtension->operators ) {
        if ( operator_->attribute->index == BPMNOS::Model::Status::Index::Timestamp ) {
          throw std::runtime_error("StateMachine: Operator '" + operator_->id + "' for subprocess '" + node->id + "' attempts to modify timestamp");
        }
        operator_->apply(status);
      }
    }
  }
//std::cerr << "advancedToEntered!" << std::endl;

  update(State::ENTERED);
//std::cerr << "updatedToEntered" << std::endl;

//std::cerr << jsonify().dump() << std::endl;

  // only check feasibility for processes and activities
  // feasibility of all other tokens must have been validated before 
  // (also for newly created or merged tokens)
  if ( !node || node->represents<BPMN::Activity>() ) {
    // check restrictions
    if ( !isFeasible() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), owner, weak_from_this());
      return;
    }
    
    // tokens entering an activity automatically
    // advance to busy state
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), owner, weak_from_this());
  }
  else if ( node->represents<BPMN::CatchEvent>() && node->incoming.size()
  ) {
    // tokens entering a catching event automatically
    // advance to busy state
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), owner, weak_from_this());
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    // tokens entering an event-based gateway automatically
    // advance to busy state
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToBusy,this), owner, weak_from_this());
  }
  else if ( node->represents<BPMN::ErrorEndEvent>() ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), owner, weak_from_this());
    return;
  }
  else {
    if ( node->represents<BPMN::EscalationThrowEvent>() ) {
      // update status and delegate control to state machine
      // if applicable, control will be delgated back to token 
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,const_cast<StateMachine*>(owner),this), owner, weak_from_this());
    }

    // tokens entering any other node automatically advance to done or
    // departed state
    if ( node->outgoing.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), owner, weak_from_this());
      return;
    }
    advanceToDeparting();
  }
}

void Token::advanceToBusy() {
//std::cerr << "advanceToBusy" << std::endl;
  update(State::BUSY);

  if ( !node ) {
    // token is at process
    auto scope = owner->process->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), owner, weak_from_this());
    }
    else {
      if ( scope->startNodes.size() == 1 ) {
        // create child statemachine
        auto engine = const_cast<Engine*>(owner->systemState->engine);
        engine->commands.emplace_back(std::bind(&StateMachine::createChild,const_cast<StateMachine*>(owner),this,scope), owner, weak_from_this());
      }
      return;
    }
  }
  else if ( node->represents<BPMN::SubProcess>() ) {
    auto scope = node->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), owner, weak_from_this());
    }
    else {
      if ( scope->startNodes.size() == 1 ) {
        // create child statemachine
        auto engine = const_cast<Engine*>(owner->systemState->engine);
        engine->commands.emplace_back(std::bind(&StateMachine::createChild,const_cast<StateMachine*>(owner),this,scope), owner, weak_from_this());
      }
      return;
    }
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    awaitEventBasedGateway();
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
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,this), owner, weak_from_this());
    }
  }
  else if ( node->represents<BPMN::MessageCatchEvent>() ) {
    awaitMessageDelivery();
  }
  else if ( node->represents<BPMN::Task>() ) {
    if ( node->represents<BPMNOS::Model::DecisionTask>() ) {
      awaitChoiceEvent();
    }
    else if ( node->represents<BPMN::ReceiveTask>() ) {
      throw std::runtime_error("Token: receive tasks are not supported");
    }
    else {
      // TODO: apply operators for completion status
      if ( auto statusExtension = node->extensionElements->as<BPMNOS::Model::Status>(); 
           statusExtension
      ) {
        for ( auto& operator_ : statusExtension->operators ) {
          operator_->apply(status);
        }
      }

//std::cerr << "->awaitTaskCompletionEvent" << std::endl;
      awaitTaskCompletionEvent();
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
//std::cerr << "advanceToCompleted" << std::endl;
  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    if ( node ) {
      throw std::runtime_error("Token: completion timestamp at node '" + node->id + "' is larger than current time");
    }
    else {
      throw std::runtime_error("Token: completion timestamp for process '" + owner->process->id + "' is larger than current time");
    }
  }

  update(State::COMPLETED);

  if ( node && node->represents<BPMN::Activity>() ) {
    awaitExitEvent();
  }
  else {
    // check restrictions
    if ( !isFeasible() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), owner, weak_from_this());
      return;
    }

    if ( !node || node->outgoing.empty() ) {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), owner, weak_from_this());
      return;
    }
    advanceToDeparting();
  }
}

void Token::advanceToExiting() {
//std::cerr << "advanceToExiting" << std::endl;
  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: exit timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::EXITING);

  // check restrictions
  if ( !isFeasible() ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), owner, weak_from_this());
    return;
  }

  if ( node->outgoing.empty() ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDone,this), owner, weak_from_this());
    return;
  }
  advanceToDeparting();
}

void Token::advanceToDone() {
//std::cerr << "advanceToDone" << std::endl;
  update(State::DONE);
  awaitStateMachineCompletion();

  const_cast<StateMachine*>(owner)->attemptShutdown();
}

void Token::advanceToDeparting() {
//std::cerr << "advanceToDeparting" << std::endl;

  if ( node->outgoing.size() == 1 ) {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,node->outgoing.front()), owner, weak_from_this());
    return;
  }

  if ( !node->represents<BPMN::Gateway>() ) {
    throw std::runtime_error("Token: implicit split at node '" + node->id + "'");
  }

  if ( auto exclusiveGateway = node->represents<BPMN::ExclusiveGateway>(); exclusiveGateway ) {
    for ( auto sequenceFlow : node->outgoing ) {
      if ( sequenceFlow != exclusiveGateway->defaultFlow ) {
        // check gatekeeper conditions
        if ( auto gatekeeper = sequenceFlow->extensionElements->as<BPMNOS::Model::Gatekeeper>(); gatekeeper ) {
          if ( gatekeeper->restrictionsSatisfied(status) ) {
            auto engine = const_cast<Engine*>(owner->systemState->engine);
            engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,sequenceFlow), owner, weak_from_this());
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
      engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,this,exclusiveGateway->defaultFlow), owner, weak_from_this());
    }
    else {
      auto engine = const_cast<Engine*>(owner->systemState->engine);
      engine->commands.emplace_back(std::bind(&Token::advanceToFailed,this), owner, weak_from_this());
    }
  }
  else {
    auto engine = const_cast<Engine*>(owner->systemState->engine);
    engine->commands.emplace_back(std::bind(&StateMachine::copyToken,const_cast<StateMachine*>(owner),this), owner, weak_from_this());
  }
}

void Token::advanceToDeparted(const BPMN::SequenceFlow* sequenceFlow) {
  this->sequenceFlow = sequenceFlow;
  update(State::DEPARTED);
  auto engine = const_cast<Engine*>(owner->systemState->engine);
  engine->commands.emplace_back(std::bind(&Token::advanceToArrived,this), owner, weak_from_this());
}

void Token::advanceToArrived() {
//std::cerr << "advanceToArrived" << std::endl;
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
    engine->commands.emplace_back(std::bind(&Token::advanceToEntered,this), owner, weak_from_this());
  }
}

void Token::advanceToFailed() {
  update(State::FAILED);
  auto engine = const_cast<Engine*>(owner->systemState->engine);
  engine->commands.emplace_back(std::bind(&StateMachine::handleFailure,const_cast<StateMachine*>(owner),this), owner, weak_from_this());
}

void Token::awaitReadyEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
//  systemState->tokensAwaitingReadyEvent.push_back(this);
  systemState->tokensAwaitingReadyEvent.push_back(weak_from_this());
}

void Token::awaitEntryEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);

  if ( auto tokenAtResource = getResourceToken(); tokenAtResource ) {
    systemState->tokensAwaitingJobEntryEvent[tokenAtResource].push_back(weak_from_this());    
  }
  else {
    systemState->tokensAwaitingRegularEntryEvent.push_back(weak_from_this());
  }
}

void Token::awaitChoiceEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingChoiceEvent.push_back(weak_from_this());
}

void Token::awaitTaskCompletionEvent() {
// TODO: apply operators, but do not change status yet
  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto time = status[BPMNOS::Model::Status::Index::Timestamp].value(); 

  systemState->tokensAwaitingTaskCompletionEvent.emplace(time,weak_from_this());
}

void Token::awaitResourceShutdownEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingResourceShutdownEvent.push_back(weak_from_this());
}

void Token::awaitExitEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingExitEvent.push_back(weak_from_this());
}

void Token::awaitTimer(BPMNOS::number time) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingTimer.emplace(time,weak_from_this());
}


void Token::awaitMessageDelivery() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingMessageDelivery.push_back(weak_from_this());
}

void Token::awaitEventBasedGateway() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingEventBasedGateway.push_back(weak_from_this());
}


void Token::awaitStateMachineCompletion() {
//std::cerr << "awaitStateMachineCompletion: " << (node ? node->id : std::string("N/A") ) << std::endl;
  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto ownerIt = systemState->tokensAwaitingStateMachineCompletion.find(owner);
  if (ownerIt == systemState->tokensAwaitingStateMachineCompletion.end()) {
    // The key is not found, so insert a new entry and get an iterator to it.
    ownerIt = systemState->tokensAwaitingStateMachineCompletion.insert({owner,{}}).first;
  }

  auto& [key,tokens] = *ownerIt;
  tokens.push_back(this);
}


void Token::awaitGatewayActivation() {
//std::cerr << "awaitGatewayActivation" << std::endl;

  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto gatewayIt = systemState->tokensAwaitingGatewayActivation.find({owner, node});
  if (gatewayIt == systemState->tokensAwaitingGatewayActivation.end()) {
    // The key is not found, so insert a new entry and get an iterator to it.
    gatewayIt = systemState->tokensAwaitingGatewayActivation.insert({{owner, node},{}}).first;
  }

  auto& [key,tokens] = *gatewayIt;
  tokens.push_back(this);
}

void Token::destroy() {
  if ( !node && node->represents<BPMNOS::Model::ResourceActivity>() ) {
    const_cast<SystemState*>(owner->systemState)->tokensAwaitingJobEntryEvent.erase(this);
  }
/*
  auto systemState = const_cast<SystemState*>(owner->systemState);
  if ( state == State::ARRIVED ) {
    if ( node->represents<BPMN::ParallelGateway>() || node->represents<BPMN::InclusiveGateway>() ) {
      auto it = systemState->tokensAwaitingGatewayActivation.find({owner, node});
      if (it != systemState->tokensAwaitingGatewayActivation.end()) {
        erase_ptr<Token>(it->second,this);
        if ( it->second.empty() ) {
          systemState->tokensAwaitingGatewayActivation.erase(it);
        }
      }
      else {
        throw std::logic_error("Token: cannot find gateway that token was waiting for");
      }
    }
    else if ( node->represents<BPMN::Activity>() ) {
//      erase_ptr<Token>(systemState->tokensAwaitingReadyEvent,this);
    }
  }
  else if ( state == State::READY ) {
    if ( auto tokenAtResource = getResourceToken(); tokenAtResource ) {
      auto it = systemState->tokensAwaitingJobEntryEvent.find(tokenAtResource);
      if (it != systemState->tokensAwaitingJobEntryEvent.end()) {
        erase_ptr<Token>(it->second,this);
      }
      // TODO: erase systemState->tokensAwaitingJobEntryEvent[tokenAtResource] when empty?
    }
    else {
      erase_ptr<Token>(systemState->tokensAwaitingRegularEntryEvent,this);
    }
  }
  else if ( state == State::BUSY ) {
    if ( !node || node->represents<BPMN::SubProcess>() ) {
      // TODO: event-subprocesses and boundary events
      if ( node->represents<BPMNOS::Model::ResourceActivity>() ) {
        // TODO
//        erase_ptr<Token>(systemState->tokensAtIdleResources,this);
//        erase_ptr<Token>(systemState->tokensAtActiveResources,this);
        systemState->tokensAwaitingJobEntryEvent.erase(this);
        erase_ptr<Token>(systemState->tokensAwaitingResourceShutdownEvent,this);
      }
    }
    else if ( node->represents<BPMN::Task>() ) {
      if ( node->represents<BPMNOS::Model::DecisionTask>() ) {
        erase_ptr<Token>(systemState->tokensAwaitingChoiceEvent,this);
      }
      else {
        erase_pair<BPMNOS::number,Token,SystemState::ScheduledTokenComparator>(systemState->tokensAwaitingTaskCompletionEvent,this);
      }
    }
    else if ( node->represents<BPMN::TimerCatchEvent>() ) {
        erase_pair<BPMNOS::number,Token,SystemState::ScheduledTokenComparator>(systemState->tokensAwaitingTimer,this);
    }
    else if ( node->represents<BPMN::MessageCatchEvent>() ) {
      erase_ptr<Token>(systemState->tokensAwaitingMessageDelivery,this);
    }
    else if ( node->represents<BPMN::EventBasedGateway>() ) {
      erase_ptr<Token>(systemState->tokensAwaitingEventBasedGateway,this);
    }
  }
  else if ( state == State::COMPLETED ) {
    if ( node && node->represents<BPMN::Activity>() ) {
      erase_ptr<Token>(systemState->tokensAwaitingExitEvent,this);
    }
  }
  else if ( state == State::DONE ) {
    auto it = systemState->tokensAwaitingStateMachineCompletion.find(owner);
    if (it != systemState->tokensAwaitingStateMachineCompletion.end()) {
      erase_ptr<Token>(it->second,this);
    }
    else {
      throw std::logic_error("Token: cannot find state machine that token is belonging to");
    }

        // TODO
//    erase_ptr<Token>(systemState->tokensAwaitingStateMachineCompletion,this);
//  std::unordered_map< const StateMachine*, std::vector<Token*> > tokensAwaitingStateMachineCompletion; ///< Map holding all tokens awaiting the completion of a state machine
  }
*/
}

Token* Token::getResourceToken() const {
  if ( auto jobShop = node->parent->represents<BPMNOS::Model::JobShop>(); jobShop ) {
    const BPMN::FlowNode* resourceActivity = jobShop->resourceActivity->as<BPMN::FlowNode>();
    Token * token = token->owner->parentToken;
    while (token->node != resourceActivity) {
      token = token->owner->parentToken;
    }
    return token;
  }

  return nullptr;
}

void Token::update(State newState) {
  state = newState;
  auto now = owner->systemState->getTime();
  if ( status[BPMNOS::Model::Status::Index::Timestamp] < now ) {
//std::cerr << "Set timestamp to " << now << std::endl;
    // increase timestamp if necessary
    status[BPMNOS::Model::Status::Index::Timestamp] = now;
  }
  else if ( status[BPMNOS::Model::Status::Index::Timestamp] > now ) {
    throw std::runtime_error("Token: timestamp at node '" + node->id + "' is larger than current time");
  }
  notify();
}

void Token::notify() const {
  for ( auto listener : owner->systemState->engine->listeners ) {
    listener->update(this);
  }
}

void Token::mergeStatus(const Token* other) {
  for ( size_t i = 0; i < status.size(); i++ ) {
    if ( !status[i].has_value() ) {
      status[i] = other->status[i];
    }
    else if ( other->status[i].has_value() && other->status[i].value() != status[i].value() ) {
      status[i] = std::nullopt;
    }
  }
}

