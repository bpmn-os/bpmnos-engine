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
//std::cerr << "Run " << this << std::endl;
  if ( !parentToken ) {
    // token without process represents a token at a process
    tokens.push_back( std::make_unique<Token>(this,nullptr,status) );
  }
  else {
    if ( scope->startNodes.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
    }
    tokens.push_back( std::make_unique<Token>(this,scope->startNodes.front(),status) );
  }

  auto token = tokens.back().get();
  if ( token->node && token->node->represents<BPMN::Activity>() ) {
    throw std::runtime_error("StateMachine: start node within scope of '" + scope->id + "' is an activity");
  }
  // advance token as far as possible
  advanceToken(token,Token::State::ENTERED);
}

void StateMachine::advanceToken(Token* token, Token::State state) {
//std::cerr << "advanceToken" << std::endl;

  if ( state == Token::State::DEPARTED) {
    token->advanceToDeparted(token->sequenceFlow);
  }
  else if ( state == Token::State::READY) {
    token->advanceToReady();
  }
  else if ( state == Token::State::ENTERED) {
    token->advanceToEntered();
  }
  else if ( state == Token::State::COMPLETED) {
    token->advanceToCompleted();
  }
  else if ( state == Token::State::EXITING) {
    token->advanceToExiting();
  }
  else {
    throw std::runtime_error("Token: advance token to illegal state");
  }

  // token has advanced as much as possible, need to decide how to continue

  if ( token->state == Token::State::BUSY) {
    if ( !token->node || token->node->represents<BPMN::SubProcess>() ) {
      auto scope = token->node ? token->node->as<BPMN::Scope>() : process->as<BPMN::Scope>();
      if ( scope->startNodes.size() == 1 ) {
        // create child statemachine and advance token
        createChild(token,scope);
      }
    }
  }
  else if ( token->state == Token::State::TO_BE_COPIED) {
    if ( token->node->represents<BPMN::ParallelGateway>() ) {
      // create token copies and advance them
      createTokenCopies(token, token->node->outgoing);
    }
    else {
      // TODO: determine sequence flows that receive a token
      throw std::runtime_error("Token: diverging gateway type not yet supported");
    }
  }
  else if ( token->state == Token::State::HALTED) {
    if ( token->node->represents<BPMN::ParallelGateway>() ) {

      auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.find({this, token->node});
      auto& [key,arrivedTokens] = *gatewayIt;
      if ( arrivedTokens.size() == token->node->incoming.size() ) {
        // create merged token and advance it
        createMergedToken(gatewayIt);
      }
    }
    else {
      // TODO: determine sequence flows that have a token
      throw std::runtime_error("Token: converging gateway type not yet supported");
    }
  }
  else if ( token->state == Token::State::FAILED) {
    throw std::runtime_error("Token: failed token not implemented");
  }
  else if ( token->state == Token::State::DONE) {
    // TODO
    auto it = const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.find(this);
    if ( it == const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.end() ) {
      throw std::logic_error("StateMachine: cannot find tokens awaiting state machine completion");
    }

    auto& [key,completedTokens] = *it;

    if ( eventSubprocessInstances.size() ) {
      throw std::runtime_error("StateMachine: event subprocesses are not yet supported");
    }

    if ( completedTokens.size() > tokens.size() ) {
std::cerr << completedTokens.size() << " > " << tokens.size() << " for " << scope->id << std::endl;
for (auto completedToken : completedTokens) {
std::cerr << completedToken << " / " << completedToken->jsonify().dump() << std::endl;
}
      throw std::logic_error("StateMachine: too many tokens");
    }

    if ( completedTokens.size() == tokens.size() ) {
      shutdown(it);
    }
  }
  else {
    // nothing to be done here because token waits for event
  }

}

void StateMachine::createChild(Token* parentToken, const BPMN::Scope* scope) {
  if ( scope->startNodes.size() > 1 ) {
    throw std::runtime_error("Token: scope '" + scope->id + "' has multiple start nodes");
  }

  childInstances.push_back(std::make_unique<StateMachine>(systemState, scope, parentToken));
  childInstances.back()->run(parentToken->status);
}

void StateMachine::createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows) {
  std::vector<Token*> tokenCopies;
  // create a token copy for each new destination
  for ( [[maybe_unused]] auto _ : sequenceFlows ) {
    tokens.push_back( std::make_unique<Token>( token ) );
    tokenCopies.push_back(tokens.back().get());
  }

  // remove original token
  erase_ptr<Token>(tokens,token);

  // advance all token copies
  for (size_t i = 0; i < sequenceFlows.size(); i++ ) {
    tokenCopies[i]->sequenceFlow = sequenceFlows[i];
    advanceToken(tokenCopies[i], Token::State::DEPARTED);
  }
}

void StateMachine::createMergedToken(std::unordered_map< std::pair<const StateMachine*, const BPMN::FlowNode*>, std::vector<Token*> >::iterator gatewayIt) {
  auto& [key,arrivedTokens] = *gatewayIt;

  // create merged token
  std::unique_ptr<Token> mergedToken = std::make_unique<Token>(arrivedTokens);

  // remove tokens
  for ( auto arrivedToken : arrivedTokens ) {
    erase_ptr<Token>(tokens,arrivedToken);
  }
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(gatewayIt);

  // add merged token
  tokens.push_back(std::move(mergedToken));

  // advance merged token
  advanceToken(tokens.back().get(), Token::State::ENTERED);
}

void StateMachine::shutdown(std::unordered_map<const StateMachine*, std::vector<Token*> >::iterator it) {
  auto& [key,completedTokens] = *it;

  if ( !parentToken ) {
    // remove instance from system state
    const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(it);
    const_cast<SystemState*>(systemState)->completedInstances.push_back(this);

    return;
  }

  // merge tokens
  for ( auto& value : parentToken->status ) {
    value = std::nullopt;
  }

  for ( auto completedToken : completedTokens ) {
    parentToken->mergeStatus(completedToken);
    erase_ptr<Token>(tokens,completedToken);
  }

  const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(it);

  const_cast<SystemState*>(systemState)->completedChildInstances.push_back(this);
}

