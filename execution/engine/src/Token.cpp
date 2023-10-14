#include "Token.h"
#include "StateMachine.h"
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

Token::Token(const StateMachine* owner, const Values& status)
  : owner(owner)
  , status(status)
  , state(State::CREATED)
{
  if ( owner->scope->startNodes.size() != 1 ) {
    throw std::runtime_error("Token: no unique start node within scope of '" + owner->scope->id + "'");
  }
  node = owner->scope->startNodes[0];

  // advance token as far as possible
  run();
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



