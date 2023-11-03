#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/extensionElements/Timer.h"
#include "model/parser/src/JobShop.h"
#include "model/parser/src/ResourceActivity.h"
#include "model/parser/src/DecisionTask.h"
#include "execution/listener/src/Listener.h"

using namespace BPMNOS::Execution;

Token::Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status)
  : owner(owner)
  , node(node)
  , state(State::CREATED)
  , status(status)
{
}

Token::Token(const Token* other) 
  : owner(other->owner)
  , node(other->node)
  , state(other->state)
  , status(other->status)
{
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

void Token::advanceToReady(std::optional< std::reference_wrapper<const Values> > values) {
//std::cerr << "advanceToReady" << std::endl;
  if ( values.has_value() ) {
    status.insert(status.end(), values.value().get().begin(), values.value().get().end());
  }

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: ready timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::READY);

//std::cerr << "->awaitEntryEvent" << std::endl;
  awaitEntryEvent();
}

void Token::advanceToEntered(std::optional< std::reference_wrapper<const Values> > statusUpdate) {
//std::cerr << "advanceToEntered" << std::endl;
  if ( statusUpdate.has_value() ) {
    status = statusUpdate.value().get();
  }

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

  update(State::ENTERED);

  // only check feasibility for processes and activities
  // feasibility of all other tokens must have been validated before 
  // (also for newly created or merged tokens)
  if ( !node || node->represents<BPMN::Activity>() ) {
    // check restrictions
    if ( !isFeasible() ) {
      advanceToFailed();
      return;
    }
    
    // tokens entering an activity automatically
    // advance to busy state
    advanceToBusy();
  }
  else if ( node->represents<BPMN::CatchEvent>() && !node->represents<BPMN::UntypedStartEvent>()
  ) {
    // tokens entering a catching event automatically
    // advance to busy state
    advanceToBusy();
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    // tokens entering an event-based gateway automatically
    // advance to busy state
    advanceToBusy();
  }
  else {
    // tokens entering any other node automatically advance to done or
    // departed state
    if ( node->outgoing.empty() ) {
      advanceToDone();
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
      advanceToCompleted();
    }
    else {
      // Delegate creation of children back to state machine
      const_cast<StateMachine*>(owner)->createChild(this,scope);

      // Wait for all tokens within the scope to complete
      awaitStateMachineCompletion();
    }
  }
  else if ( node->represents<BPMN::SubProcess>() ) {
    auto scope = node->as<BPMN::Scope>();
    if ( scope->startNodes.empty() ) {
      advanceToCompleted();
    }
    else {
      // Delegate creation of children back to state machine
      const_cast<StateMachine*>(owner)->createChild(this,scope);

      // Wait for all tokens within the scope to complete
      awaitStateMachineCompletion();
    }
  }
  else if ( node->represents<BPMN::EventBasedGateway>() ) {
    awaitEventBasedGateway();
  }
  else if ( node->represents<BPMN::TimerCatchEvent>() ) {
    // TODO: determine time
    auto trigger = node->extensionElements->as<BPMNOS::Model::Timer>()->trigger.get();
    BPMNOS::number time;

    if ( trigger->attribute && status[trigger->attribute.value().get().index].has_value() ) {
      time = status[trigger->attribute.value().get().index].value();
    }
    else if ( trigger->value && trigger->value.has_value() ) {
      time = (int)trigger->value.value().get();
    }
    else {
      throw std::runtime_error("Token: no trigger given for node '" + node->id + "'");
    }

    if ( time > owner->systemState->getTime() ) {
      awaitTimer(time);
    }
    else {
      advanceToCompleted();
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

void Token::advanceToCompleted(const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues) {
  for ( auto & [index, value] : updatedValues ) {
    status[index] = value;
  }
  advanceToCompleted();
}

void Token::advanceToCompleted(const Values& statusUpdate) {
  status = statusUpdate;
  advanceToCompleted();
}

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
      advanceToFailed();
      return;
    }

    if ( !node || node->outgoing.empty() ) {
      advanceToDone();
      return;
    }
    advanceToDeparting();
  }
}

void Token::advanceToExiting(std::optional< std::reference_wrapper<const Values> > statusUpdate) {
//std::cerr << "advanceToExiting" << std::endl;
  if ( statusUpdate.has_value() ) {
    status = statusUpdate.value().get();
  }

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: exit timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::EXITING);

  // check restrictions
  if ( !isFeasible() ) {
    advanceToFailed();
    return;
  }

  if ( node->outgoing.empty() ) {
    advanceToDone();
    return;
  }
  advanceToDeparting();
}

void Token::advanceToDone() {
  update(State::DONE);
  // delegate disposal to state machine
  const_cast<StateMachine*>(owner)->awaitTokenDisposal(this);
}

void Token::advanceToDeparting() {

  if ( node->outgoing.size() == 1 ) {
    advanceToArrived(node->outgoing.front()->target);
    return;
  }
  
  // TODO: determine sequence flows that receive a token
  throw std::runtime_error("Token: diverging gateways not yet supported");
/*
  if ( false ) {
    // no sequence flow satisfies conditions
    advanceToFailed();
    return;
  }

  if ( false ) {
    // let state machine copy token and advance each copy
//    owner->copy(this);
//  advanceToArrived(destination);
    return;
  }
*/

}


void Token::advanceToArrived(const BPMN::FlowNode* destination) {
//std::cerr << "advanceToArrived" << std::endl;
  node = destination;
  update(State::ARRIVED);

  // TODO: check whether gateway activation is required
  if ( node->incoming.size() > 1 
       && node->represents<BPMN::Gateway>()
       && !node->represents<BPMN::ExclusiveGateway>()
  ) {
    awaitGatewayActivation();    
  }

  if ( node->represents<BPMN::Activity>() ) {
    awaitReadyEvent();
  }
  else {
    advanceToEntered();
  }
}

void Token::advanceToFailed() {
  update(State::FAILED);
  // TODO
}


void Token::awaitReadyEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingReadyEvent.push_back(this);
}

void Token::awaitEntryEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);

  if ( auto tokenAtResource = getResourceToken(); tokenAtResource ) {
    systemState->tokensAwaitingJobEntryEvent[tokenAtResource].push_back(this);    
  }
  else {
    systemState->tokensAwaitingRegularEntryEvent.push_back(this);
  }
}

void Token::awaitChoiceEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingChoiceEvent.push_back(this);
}

void Token::awaitTaskCompletionEvent() {
// TODO: apply operators, but do not change status yet
  auto systemState = const_cast<SystemState*>(owner->systemState);
  auto time = status[BPMNOS::Model::Status::Index::Timestamp].value(); 
  systemState->tokensAwaitingTaskCompletionEvent.insert({time,this});
}

void Token::awaitResourceShutdownEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingResourceShutdownEvent.push_back(this);
}

void Token::awaitExitEvent() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingExitEvent.push_back(this);
}

void Token::awaitTimer(BPMNOS::number time) {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingTimer.push({time,this});
}


void Token::awaitMessageDelivery() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingMessageDelivery.push_back(this);
}

void Token::awaitEventBasedGateway() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingEventBasedGateway.push_back(this);
}


void Token::awaitStateMachineCompletion() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingStateMachineCompletion[owner].push_back(this);
}


void Token::awaitGatewayActivation() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingGatewayActivation[node].push_back(this);
}

/*
void Token::awaitDisposal() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingDisposal[owner].push_back(this);
}
*/


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

