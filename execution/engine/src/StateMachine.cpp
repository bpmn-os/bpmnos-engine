#include "StateMachine.h"
#include "SystemState.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "execution/utility/src/erase.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Process* process)
  : systemState(systemState)
  , process(process)
  , scope(process)
  , parentToken(nullptr)
{
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
{
}

void StateMachine::run(const Values& status) {
  if ( !parentToken ) {
    // token without process represents a token at a process
    tokens.push_back( Token(this,nullptr,status) );
  }
  else {
    if ( scope->startNodes.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
    }
    tokens.push_back( Token(this,scope->startNodes.front(),status) );
  }
  // advance token as far as possible
  tokens.back().advanceFromCreated();
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

void StateMachine::createChild(Token* parentToken, const BPMN::Scope* scope) {
  if ( scope->startNodes.size() > 1 ) {
    throw std::runtime_error("Token: scope '" + scope->id + "' has multiple start nodes");
  }

  childInstances.push_back(StateMachine(systemState, scope, parentToken));
  childInstances.back().run(parentToken->status);
}

void StateMachine::attemptGatewayActivation(std::unordered_map< std::pair< const StateMachine*, const BPMN::FlowNode*>, std::vector<Token*> >::iterator gatewayIt) {
std::cerr << "attemptGatewayActivation" << std::endl;
  auto& [key,arrivedTokens] = *gatewayIt;
  auto& [owner,node] = key;

  if ( node->represents<BPMN::ParallelGateway>() ) {
    if ( arrivedTokens.size() < node->incoming.size() ) {
      return;
    }
  }
  else {
    throw std::runtime_error("Token: unsupported converging behaviour at node '" + node->id + "'");
  } 

  tokens.push_back( Token( arrivedTokens.front() ) );
  auto& token = tokens.back();

  for ( auto arrivedToken : arrivedTokens ) {
    erase_ptr<Token>(tokens,arrivedToken);
  }
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(gatewayIt);

  token.advanceToEntered();
}

void StateMachine::attemptStateMachineCompletion(std::unordered_map< const StateMachine*, std::vector<Token*> >::iterator it) {
std::cerr << "attemptStateMachineCompletion" << std::endl;
  auto& [key,completedTokens] = *it;

  if ( eventSubprocessInstances.size() ) {
    throw std::runtime_error("StateMachine: event subprocesses are not yet supported");
  }

  if ( completedTokens.size() < tokens.size() ) {
    return;
  }

  if ( !parentToken ) {
    throw std::logic_error("StateMachine: attemptStateMachineCompletion without parent");
  }

  for ( auto& value : parentToken->status ) {
    value = std::nullopt;
  }

  for ( auto completedToken : completedTokens ) {
    parentToken->mergeStatus(completedToken);
    erase_ptr<Token>(tokens,completedToken);
  }

  const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(it);

  parentToken->advanceToCompleted();

}

/*
void StateMachine::awaitTokenDisposal(Token* token) {

  auto& tokensAwaitingDisposal = const_cast<SystemState*>(systemState)->tokensAwaitingDisposal[this];

  tokensAwaitingDisposal.push_back(token);

  if ( tokensAwaitingDisposal.size() < tokens.size() ) {
    return;
  }

  // all tokens in scope are done (thus, no child instance is running)

  // TODO: check for running event subprocesses

  // TODO: merge tokens 
  Values status = token->status; // TODO: resize if necessary!
  const_cast<SystemState*>(systemState)->tokensAwaitingDisposal.erase(this);

  if ( parentToken ) {
    // (sub)process is completed, resume with paren token
    parentToken->advanceToCompleted(status);
  }
  //const Token* parentToken = token->owner->parentToken;

  if ( !parentToken ) {
//    const_cast<SystemState*>(systemState)->deleteInstance(this);
  }
}
*/

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

