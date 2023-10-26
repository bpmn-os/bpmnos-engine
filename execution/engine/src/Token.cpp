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
  notify();

  // advance token as far as possible
  // TODO: advanceFromCreated();
}

Token::Token(const Token* other) 
  : owner(other->owner)
  , node(other->node)
  , state(other->state)
  , status(other->status)
{
}

nlohmann::json Token::jsonify() const {
  nlohmann::json jsonObject;
  jsonObject["processId"] = owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string(status[Model::Status::Index::Instance].value(),STRING);
  if ( node ) {
    jsonObject["nodeId"] = node->id;
  }
  jsonObject["state"] = stateName[(int)state];
  jsonObject["status"] = nlohmann::json::object();

  auto& attributeMap = ( node ? node->extensionElements->as<const Model::Status>()->attributeMap : owner->process->extensionElements->as<const Model::Status>()->attributeMap ); 

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
  if ( node->is<BPMN::Activity>() ) {
    awaitReadyEvent();
  }
  else {
//TODO    advanceToEntered();
  }
}

void Token::advanceToReady(std::optional< std::reference_wrapper<const Values> > values) {
  if ( values.has_value() ) {
    status.insert(status.end(), values.value().get().begin(), values.value().get().end());
  }

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: ready timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::READY);

  awaitEntryEvent();
}

void Token::advanceToEntered(std::optional< std::reference_wrapper<const Values> > statusUpdate) {
  if ( statusUpdate.has_value() ) {
    status = statusUpdate.value().get();
  }

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: entry timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::ENTERED);

  // only check feasibility for activities
  // feasibility of all other tokens must have been validated before 
  // (also for newly created or merged tokens)
  if ( node->is<BPMN::Activity>() ) {
    // check restrictions
    if ( !isFeasible() ) {
      advanceToFailed();
      return;
    }
    
    // tokens entering an activity automatically
    // advance to busy state
    advanceToBusy();
  }
  else if ( node->is<BPMN::CatchEvent>() 
  ) {
    // tokens entering a catching event automatically
    // advance to busy state
    advanceToBusy();
  }
  else if ( node->is<BPMN::EventBasedGateway>() ) {
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
  update(State::BUSY);
  if ( node->is<BPMN::EventBasedGateway>() ) {
    awaitEventBasedGateway();
  }
  else if ( node->is<BPMN::TimerCatchEvent>() ) {
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
  else if ( node->is<BPMN::MessageCatchEvent>() ) {
    awaitMessageDelivery();
  }
  else if ( node->is<BPMN::Task>() ) {
    if ( node->is<BPMNOS::Model::DecisionTask>() ) {
      awaitChoiceEvent();
    }
    else if ( node->is<BPMN::ReceiveTask>() ) {
      throw std::runtime_error("Token: receive tasks are not supported");
    }
    else {
      awaitTaskCompletionEvent();
    }
  }
  else if ( node->is<BPMN::SubProcess>() ) {
    // TODO: Delegate creation of children back to state machine
    awaitSubProcessCompletion();
  }
}

void Token::advanceToCompleted(const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues) {
  for ( auto & [index, value] : updatedValues ) {
    status[index] = value;
  }

  if ( status[BPMNOS::Model::Status::Index::Timestamp] > owner->systemState->getTime() ) {
    throw std::runtime_error("Token: completion timestamp at node '" + node->id + "' is larger than current time");
  }

  update(State::COMPLETED);

  if ( node->is<BPMN::Activity>() ) {
    awaitExitEvent();
  }
  else {
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
}

void Token::advanceToExiting(std::optional< std::reference_wrapper<const Values> > statusUpdate) {
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
  awaitDisposal();
}

void Token::advanceToDeparting() {
  // TODO: determine sequence flows that receive a token
  
  if ( false ) {
    // no sequence flow satisfies conditions
    advanceToFailed();
    return;
  }

  if ( false ) {
    // let state machine copy token and advance each copy
//    owner->copy(this);
    return;
  }

  const BPMN::FlowNode* destination = nullptr; //TODO
  advanceToArrived(destination);
}


void Token::advanceToArrived(const BPMN::FlowNode* destination) {
  node = destination;
  update(State::ARRIVED);

  // TODO: check whether gateway activation is required
  if ( node->incoming.size() > 1 
       && node->is<BPMN::Gateway>()
       && !node->is<BPMN::ExclusiveGateway>()
  ) {
    awaitGatewayActivation();    
  }

  if ( node->is<BPMN::Activity>() ) {
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


void Token::awaitSubProcessCompletion() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingSubProcessCompletion[owner].push_back(this);
}


void Token::awaitGatewayActivation() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingGatewayActivation[node].push_back(this);
}

void Token::awaitDisposal() {
  auto systemState = const_cast<SystemState*>(owner->systemState);
  systemState->tokensAwaitingDisposal[owner].push_back(this);
}


Token* Token::getResourceToken() {
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

/*
void Token::processEvent(const Event* event) {
  Token* token = const_cast<Token*>(event->token);

  if ( auto entryEvent = event->is<EntryEvent>(); entryEvent ) {
      if ( token->node->is<BPMN::SubProcess>() ) {
        // update token state
        token->state = Token::State::BUSY;

        // apply operators
        if ( auto statusExtension = token->node->extensionElements->as<BPMNOS::Model::Status>(); 
             statusExtension
        ) {
          for ( auto& operator_ : statusExtension->operators ) {
            if ( operator_->attribute->index == BPMNOS::Model::Status::Index::Timestamp ) {
              throw std::runtime_error("StateMachine: Operator '" + operator_->id + "' for subprocess '" + token->node->id + "' attempts to modify timestamp");
            }

            operator_->apply(token->status);
          }
        }

        // check restrictions
        if ( false ) {
          token->state = Token::State::FAILED;
        }
      }
      else if ( token->node->is<BPMN::Task>() ) {
      }
      else {
      }
  }
}

*/

