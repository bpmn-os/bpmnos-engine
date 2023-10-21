#include "Token.h"
#include "StateMachine.h"
#include "Engine.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/DecisionTask.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "events/ExitEvent.h"
#include "events/ChoiceEvent.h"
#include "events/CompletionEvent.h"
#include "events/TriggerEvent.h"
#include "events/MessageDeliveryEvent.h"

using namespace BPMNOS::Execution;

Token::Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status)
  : owner(owner)
  , node(node)
  , state(State::CREATED)
  , status(status)
{
  notify();
  // advance token as far as possible
  run();
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
//      jsonObject["status"].push_back(nlohmann::json({{attributeName,nullptr}}));
      jsonObject["status"][attributeName] = nullptr ;
    }
    else if ( attribute->type == STRING) {
      std::string value = BPMNOS::to_string(status[attribute->index].value(),STRING);
//      jsonObject["status"].push_back(nlohmann::json({{attributeName,value}}));
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == BOOLEAN) {
      bool value = (bool)status[attribute->index].value();
//      jsonObject["status"].push_back(nlohmann::json({{attributeName,value}}));
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == INTEGER) {
      int value = (int)status[attribute->index].value();
//      jsonObject["status"].push_back(nlohmann::json({{attributeName,value}}));
      jsonObject["status"][attributeName] = value ;
    }
    else if ( attribute->type == DECIMAL) {
      double value = (double)status[attribute->index].value();
//      jsonObject["status"].push_back(nlohmann::json({{attributeName,value}}));
      jsonObject["status"][attributeName] = value ;
    }
  }

  return jsonObject;
}


void Token::run() {
  // TODO: Event-subprocesses, boundary-events, error handling
  bool advance = false;
  while ( advance ) {
    if ( state == State::CREATED ) {
      advance = advanceFromCreated();
    }
    else if ( state == State::READY ) {
      advance = advanceFromReady();
    }
    else if ( state == State::ENTERED ) {
      advance = advanceFromEntered();
    }
    else if ( state == State::COMPLETED ) {
      advance = advanceFromCompleted();
    }
    else if ( state == State::DEPARTED ) {
      advance = advanceFromDeparted();
    }
    else if ( state == State::ARRIVED ) {
      advance = advanceFromArrived();
    }
    else {  // REMOVE when all cases are considered
      advance = false; // break out of loop
    }
  }
}

bool Token::advanceFromCreated() {
  return advanceToReady();
}

bool Token::advanceToReady() {
  if ( node->is<BPMN::ExclusiveGateway>() || node->incoming.size() <= 1 ) {
    if ( auto statusExtension = node->extensionElements->as<BPMNOS::Model::Status>(); 
         statusExtension && statusExtension->attributes.size()
    ) {
      // wait for ready event
      return false;
    }
    else {
      state = State::READY;
    }
  }
  else {
    state = State::TO_BE_MERGED;
    // delegate merge back to state machine
    return false;
  }
  return true;
}

bool Token::advanceFromReady() {
  if ( node->is<BPMN::Activity>() ) {
    // Need to wait for entry event
    return false;
  }

  state = State::ENTERED;

  return true;
}


bool Token::advanceFromEntered() {
  // ENTERED state
  if ( node->is<BPMN::Activity>() ) {
    if ( node->is<BPMN::SubProcess>() ) {
      // Delegate creation of children back to state machine
      return false;
    }
    else if ( node->is<BPMN::Task>() ) {
      if ( node->is<BPMNOS::Model::DecisionTask>() ) {
        // Need to wait for choice event
        return false;
      }
      else if ( node->is<BPMN::ReceiveTask>() ) {
        throw std::runtime_error("Token: receive tasks are not supported");
      }
      else {
        // All other task types are ignored

        // TODO: apply operators
        // If operators increase time wait for trigger event,
        // otherwise wait for exit event 
        return false;
      }
        }
    else {
      // CallActivity
      throw std::runtime_error("Token: call activities are not supported");
    }
  }
  else if ( node->is<BPMN::Event>() ) {
    // Need to wait for trigger event
    return false;
  }
  return true;
}

bool Token::advanceFromCompleted() {
  return true;
}

bool Token::advanceFromDeparted() {
  return true;
}

bool Token::advanceFromArrived() {
  return advanceToReady();
}

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

void Token::processExitEvent(const ExitEvent* exitEvent) {
}

void Token::processChoiceEvent(const ChoiceEvent* choiceEvent) {
}

void Token::processCompletionEvent(const CompletionEvent* completionEvent) {
}

void Token::processTriggerEvent(const TriggerEvent* triggerEvent) {
}

void Token::processMessageDeliveryEvent(const MessageDeliveryEvent* messageDeliveryEvent) {
}


void Token::update(State newState) {
  state = newState;
  notify();
}

void Token::appendToStatus(Token* token, const Values& values) {
  token->status.insert(token->status.end(), values.begin(), values.end());
}

void Token::replaceStatus(Token* token, const Values& newStatus) {
  token->status = newStatus;
}

void Token::resizeStatus(Token* token) {
  auto& attributeMap = ( token->node ? token->node->extensionElements->as<const Model::Status>()->attributeMap : token->owner->process->extensionElements->as<const Model::Status>()->attributeMap ); 

  token->status.resize(attributeMap.size());
}



void Token::notify() const {
  for ( auto listener : owner->systemState->engine->listeners ) {
    listener->update(this);
  }
}

