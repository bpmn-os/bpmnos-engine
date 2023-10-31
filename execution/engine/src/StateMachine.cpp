#include "StateMachine.h"
#include "SystemState.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "execution/utility/src/erase.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Process* process, const Values& status)
  : systemState(systemState)
  , process(process)
  , scope(process)
  , parentToken(nullptr)
{
  tokens.push_back( Token(this,nullptr,status) );
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, const Values& status, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
{
  if ( scope->startNodes.size() != 1 ) {
    throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
  }
  tokens.push_back( Token(this,scope->startNodes[0],status) );
}

/*
void StateMachine::advance() {
  for ( auto& token : tokens ) {
    advance(token);
  }
}

void StateMachine::advance(Token& token) {
//  throw std::runtime_error("StateMachine: advance token not yet implemented");
}
*/

void StateMachine::disposeToken(Token* token) {
  const Token* parentToken = token->owner->parentToken;

  // Find the iterator pointing to the token
  auto it = std::find_if(tokens.begin(), tokens.end(), [token](const Token& element) { return &element == token; });

  if (it != tokens.end()) {

    if ( tokens.empty() ) {

      if ( parentToken ) {
        // TODO
      }
      else {
        // remove state machine from system state
        const_cast<SystemState*>(systemState)->deleteInstance(this);
      }

    }

    // Element found, remove it
    tokens.erase(it);
  }
  else {
    throw std::runtime_error("StateMachine: cannot find token to be deleted");
  }

}


////////////
/*
bool StateMachine::run(const Event* event) {
  // obtain non-const token the event is referring to
  Token* token = const_cast<Token*>(event->token);

  // delegate event processing to the token
  token->processEvent(event);

  if ( auto entryEvent = event->is<EntryEvent>(); entryEvent && token->node->is<BPMN::SubProcess>() ) {
    if ( token->state == Token::State::FAILED ) {
      // TODO
      // find error catch event
    }
    else {
      // create child instance
      childInstances.push_back( StateMachine(token->node->as<BPMN::Scope>(),token->status,token) );
      bool runningChild = run( childInstances.back()->tokens.back().get() );
      if ( !runningChild ) {
        token->state = Token::State::COMPLETED;
      }
    }
  }

  return run(token);
}

bool StateMachine::run(Token* token) {
  switch (token->state) {
    case Token::State::ENTERED:
      if ( token->node->is<BPMN::SubProcess>() ) {
      }
      break;
    case Token::State::DONE:
      // check whether to wait for other tokens
      if ( isCompleted() ) {
        if ( !parentToken ) {
          // instance is completed
          return false;
        }
        continueWithParentToken();
      }
      break;
    case Token::State::FAILED:
      // TODO: bubble up
      break;
    case Token::State::TO_BE_MERGED:
      // check if merge is necessary
      break;
    case Token::State::TO_BE_COPIED:
      // TODO: create copies
      erase<Token>(tokens,token);
      break;
    default:
      break;
  }

  return true;
}

bool StateMachine::isCompleted() const {
  for ( auto& token : tokens ) {
    if ( token->state != Token::State::DONE ) {
      return false;
    }
  }
  // no runnig token remaining at flow nodes, check active event-subprocess
  for ( auto& instance : eventSubprocessInstances ) {
    if ( instance->tokens.size() ) {
      return false;
    }
  }

  return true;
}
*/

/*
void StateMachine::continueWithParentToken() {
  // update parent token status
  for ( size_t i = 0; i < parentToken->status.size(); i++ ) {
    parentToken->status[i] = std::nullopt;
    for ( auto& token : tokens ) {
      if ( !parentToken->status[i].has_value() ) {
        // parent value has not yet been set
        // use token value whether defined or undefined 
        parentToken->status[i] = token.status[i].value();
      }
      else if ( token.status[i].has_value() ) {
        // both parent value and token vale are defined
        // check that both have the same value 
        if ( parentToken->status[i].value() != token.status[i].value() ) {
          // conflicting values, use undefined instead and proceed with next value
          parentToken->status[i] = std::nullopt;
          break;
        }
      }
    }
  }
  // remove state machine from parent
  StateMachines* stateMachines = const_cast<StateMachines*>(&parentToken->owner->childInstances);
  erase<StateMachine>(*stateMachines,this);

  parentToken->run();
}
*/

