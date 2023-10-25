#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/DecisionTask.h"

using namespace BPMNOS::Execution;

Token::Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status)
  : owner(owner)
  , node(node)
  , state(State::CREATED)
  , status(status)
{
  notify();
  // advance token as far as possible
  advanceFromCreated();
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
    awaitActivatingEvent();
  }
  else if ( node->is<BPMN::MessageCatchEvent>() ) {
    awaitMessageDeliveryEvent();
  }
  else if ( node->is<BPMN::CatchEvent>() ) {
    awaitTriggerEvent();
  }
  else if ( node->is<BPMN::Task>() ) {
    if ( node->is<BPMNOS::Model::DecisionTask>() ) {
      awaitChoices();
    }
    else if ( node->is<BPMN::ReceiveTask>() ) {
      throw std::runtime_error("Token: receive tasks are not supported");
    }
    else {
      awaitCompletionEvent();
    }
  }
  else if ( node->is<BPMN::SubProcess>() ) {
    // TODO: Delegate creation of children back to state machine

    awaitSubProcessCompletion();
  }
}

void Token::advanceToCompleted(const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& updatedValues) {
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
  // TODO
}

void Token::awaitEntryEvent() {
  // TODO
}

void Token::awaitActivatingEvent() {
  // TODO
}

void Token::awaitMessageDeliveryEvent() {
  // TODO
}

void Token::awaitTriggerEvent() {
  // TODO
}

void Token::awaitChoices() {
  // TODO
}

void Token::awaitCompletionEvent() {
  // TODO
}

void Token::awaitSubProcessCompletion() {
  // TODO
}

void Token::awaitExitEvent() {
  // TODO
}

void Token::awaitDisposal() {
  // TODO
}

void Token::awaitGatewayActivation() {
  // TODO
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
        // TODO
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

