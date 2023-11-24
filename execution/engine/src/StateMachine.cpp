#include "Engine.h"
#include "StateMachine.h"
#include "Token.h"
#include "SystemState.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "execution/utility/src/erase.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/extensionElements/Timer.h"

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Process* process)
  : systemState(systemState)
  , process(process)
  , scope(process)
  , parentToken(nullptr)
  , isCompleting(false)
{
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
  , isCompleting(false)
{
}

StateMachine::StateMachine(const StateMachine* other)
  : systemState(other->systemState)
  , process(other->process)
  , scope(other->scope)
  , parentToken(other->parentToken)
  , isCompleting(false)
{
}

void StateMachine::initiateEventSubprocesses(Token* token) {
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    pendingEventSubProcesses.push_back(std::make_unique<StateMachine>(systemState, eventSubProcess, token));
    auto pendingEventSubProcess = pendingEventSubProcesses.back().get();
//std::cerr << "Pending event subprocess has parent: " << pendingEventSubProcess->parentToken->jsonify().dump() << std::endl;

    if ( pendingEventSubProcess->scope->startNodes.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node for event subprocess '" + pendingEventSubProcess->scope->id + "'");
    }

/*
    pendingEventSubProcess->tokens.push_back( std::make_unique<Token>(pendingEventSubProcess,nullptr,token->status) );

    auto pendingToken = pendingEventSubProcess->tokens.back().get();

    auto startNode = pendingEventSubProcess->scope->startNodes.front();

    if ( startNode->represents<BPMN::EscalationStartEvent>() ) {
      // nothing to do here
    }
    else if ( startNode->represents<BPMN::TimerCatchEvent>() ) {
      auto trigger = startNode->extensionElements->as<BPMNOS::Model::Timer>()->trigger.get();
      BPMNOS::number time;
      if ( trigger && trigger->attribute && token->status[trigger->attribute.value().get().index].has_value() ) {
        time = token->status[trigger->attribute.value().get().index].value();
      }
      else if ( trigger && trigger->value && trigger->value.has_value() ) {
        time = (int)trigger->value.value().get();
      }
      else {
        throw std::runtime_error("StateMachine: no trigger given for node '" + pendingEventSubProcess->scope->id + "'");
      }
     
      if ( time > systemState->getTime() ) {
        // wait for scheduled time
        pendingToken->awaitTimer(time);
      }
      else {
        // remove event subprocess because scheduled time has been in the past
        pendingEventSubProcesses.pop_back();
      }
    }
    else if ( startNode->represents<BPMN::MessageCatchEvent>() ) {
      pendingToken->awaitMessageDelivery();
    }
    else {
      throw std::runtime_error("StateMachine: unsupported type of event subprocess '" + pendingEventSubProcess->scope->id + "'");
    }
*/
  }
}

void StateMachine::run(const Values& status) {
//std::cerr << "Run " << scope->id << std::endl;
  if ( !parentToken ) {
    // state machine without parent token represents a token at a process
//std::cerr << "Start process " << process->id << std::endl;
    tokens.push_back( std::make_unique<Token>(this,nullptr,status) );
  }
  else {
    if ( scope->startNodes.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
    }
//std::cerr << "Start at " << scope->startNodes.front()->id << std::endl;
//std::cerr << "Parent: " << parentToken->jsonify().dump() << "<" << std::endl;
    tokens.push_back( std::make_unique<Token>(this,scope->startNodes.front(),status) );
//    Token t = Token(this,scope->startNodes.front(),status);
//std::cerr << scope->id << " =>" << t.jsonify().dump() << "<" << std::endl;
  }

  auto token = tokens.back().get();
//std::cerr << ">" << token->jsonify().dump() << "<" << std::endl;
  if ( token->node && token->node->represents<BPMN::Activity>() ) {
    throw std::runtime_error("StateMachine: start node within scope of '" + scope->id + "' is an activity");
  }

  // advance token as far as possible
  advanceToken(token,Token::State::CREATED);
}

template <typename Function, typename... Args>
void StateMachine::queueCommand(Function&& f, Args&&... args) {
  const_cast<Engine*>(systemState->engine)->commands.emplace_back(
    this,
    nullptr,
    std::bind(std::forward<Function>(f), this, std::forward<Args>(args)...)
  );
}

void StateMachine::advanceToken(Token* token, Token::State state) {
//std::cerr << "advanceToken " << scope->id << std::endl;

  if ( state == Token::State::CREATED) {
//std::cerr << "initiateEventSubproceses " << scope->id << std::endl;
    if ( !parentToken ) {
      // state machine is top level-process
      initiateEventSubprocesses(token);
    }
    token->advanceToEntered();
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
  else if ( state == Token::State::DEPARTED) {
    token->advanceToDeparted(token->sequenceFlow);
  }
  else {
    throw std::runtime_error("StateMachine: advance token to illegal state");
  }

  // token has advanced as much as possible, need to decide how to continue
//std::cerr << std::endl << "Continue with: " << token->jsonify().dump() << std::endl;

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
      throw std::runtime_error("StateMachine: diverging gateway type not yet supported");
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
      throw std::runtime_error("StateMachine: converging gateway type not yet supported");
    }
  }
  else if ( token->state == Token::State::ENTERED) {
    if ( token->node && token->node->represents<BPMN::EscalationThrowEvent>() ) { 
      // update status
      parentToken->status = token->status;
      parentToken->update(parentToken->state);

/*
      // TODO: find event-subprocess triggered by escalation
      auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::unique_ptr<StateMachine>& eventSubProcess) {
        auto startNode = eventSubProcess->scope->startNodes.front();
        return startNode->represents<BPMN::EscalationStartEvent>();
      });
      if ( it != pendingEventSubProcesses.end() ) {
        auto pendingEventSubProcess = it->get();
        if ( pendingEventSubProcess->scope->as<BPMN::EventSubProcess>()->isInterrupting ) {
          // TODO
          // kill child instances
          // destroy running tokens
          // start interrupting event subprocess
        }
        else {
          // TODO
          // create new instantiation of event subprocess
          // start non-interrupting event subprocess
        }
      }
      
      // TODO: find boundary event triggered by escalation
      if ( auto activity = scope->represents<BPMN::Activity>(); activity ) {
      }
*/

      if ( token->node->outgoing.empty() ) {
        advanceToken(token, Token::State::DONE);
      }
      else if ( token->node->outgoing.size() == 1) {
        token->sequenceFlow = token->node->outgoing.front();
        advanceToken(token, Token::State::DEPARTED);
      } 
      else {
        throw std::logic_error("StateMachine: implicit split for EscalationThrowEvent");
      }
    }
    else {
      // TODO: determine sequence flows that have a token
      throw std::logic_error("StateMachine: token state can only be ENTERED for EscalationThrowEvent");
    }
  }
  else if ( token->state == Token::State::FAILED) {
    // update status of parent token with that of current token
    parentToken->status = token->status;
    // find event-subprocess catching error
    auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::unique_ptr<StateMachine>& eventSubProcess) {
      auto startNode = eventSubProcess->scope->startNodes.front();
      return startNode->represents<BPMN::ErrorStartEvent>();
    });

    if ( it == pendingEventSubProcesses.end() ) {
std::cerr << "bubbble up error" << std::endl;
      // bubbble up error if not caught by event subprocess
      // TODO
/*
      terminate();
*/
      parentToken->update(Token::State::FAILED);
    }
    else {
std::cerr << "trigger error event subprocess" << std::endl;
      // start error event subprocess
//      throw std::runtime_error("StateMachine: start error event subprocess not implemented");
      // TODO
/*
       terminate();
*/
       createInterruptingEventSubprocess(it->get(),parentToken->status);
    }
  }
  else if ( token->state == Token::State::DONE) {
/*
    if ( token->node && token->node->represents<BPMN::TerminateEvent>() ) {
      // TODO: copy status to parent token
      terminate();
    }
    else {
    }
*/
    if ( token->owner->nonInterruptingEventSubProcesses.size() ) {
      // wait for completion of all non-interrupting event subürocesses
      return;
    }

    // TODO
    auto it = const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.find(this);
    if ( it == const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.end() ) {
      throw std::logic_error("StateMachine: cannot find tokens awaiting state machine completion");
    }

    auto& [key,completedTokens] = *it;

    if ( completedTokens.size() < tokens.size() ) {
      return;
    }
    else if ( completedTokens.size() == tokens.size() ) {
      // all tokens are in DONE state
      // no new event subprocesses can be triggered
      pendingEventSubProcesses.clear();
/*
      for ( auto& eventSubProcess: pendingEventSubProcesses ) {
        // remove tokens at start events from respective containers
// TODO!
        auto token = eventSubProcess->tokens.front().get();
        if ( token->node->represents<BPMN::MessageCatchEvent>() ) {
          erase<Token*>(const_cast<SystemState*>(systemState)->tokensAwaitingMessageDelivery, token);
        }
        else if ( token->node->represents<BPMN::TimerCatchEvent>() ) {
          // TODO
          throw std::runtime_error("StateMachine: timer start events not yet implemented");
        }
        else {
          throw std::runtime_error("StateMachine: illegal start event for event subprocess");
        }
      }
*/
    }
    else {
      throw std::logic_error("StateMachine: too many tokens");
    }

    if ( nonInterruptingEventSubProcesses.size() ) {
      // wait until last event subprocess is completed
      return;
    }

    if ( interruptingEventSubProcess ) {
      throw std::logic_error("StateMachine: interrupting event subprocesses hasn't removed tokens");
    }

    shutdown(it);
  }
  else {
    // nothing to be done here because token waits for event
  }

}

void StateMachine::createChild(Token* parent, const BPMN::Scope* scope) {
  if ( scope->startNodes.size() > 1 ) {
    throw std::runtime_error("StateMachine: scope '" + scope->id + "' has multiple start nodes");
  }

  subProcesses.push_back(std::make_unique<StateMachine>(systemState, scope, parent));
  auto subProcess = subProcesses.back().get();
  subProcess->initiateEventSubprocesses(parent);
  subProcess->run(parent->status);
}

void StateMachine::createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  interruptingEventSubProcess = std::make_unique<StateMachine>(pendingEventSubProcess);
  // delete pending event subprocess to ensure that they will not be triggered again
  pendingEventSubProcesses.clear();
//std::cerr << "triggered interrupting event subprocess with scope " <<  interruptingEventSubProcess->scope->id << std::endl;

  interruptingEventSubProcess->run(status);
}

/*
      interruptingEventSubProcess = std::make_unique<StateMachine>(it->get());
      auto startNode = interruptingEventSubProcess->scope->startNodes.front();
      interruptingEventSubProcess->tokens.push_back( std::make_unique<Token>( interruptingEventSubProcess.get(), startNode,token->status) );

std::cerr << "terminate " << token << "/" << interruptingEventSubProcess->tokens.back().get() <<std::endl;
std::cerr << "token: " << token->jsonify().dump() <<  std::endl;

      terminate();

std::cerr << "advance " << interruptingEventSubProcess->tokens.size() <<std::endl;
      auto pendingToken = interruptingEventSubProcess->tokens.back().get();
std::cerr << pendingToken << "/" << pendingToken->jsonify().dump() <<  std::endl;
//      interruptingEventSubProcess->advanceToken(interruptingEventSubProcess->tokens.back().get(), Token::State::CREATED);
std::cerr << "shutdown (" << interruptingEventSubProcess->scope->id << "): " << scope->id << std::endl;

      return;
//      parentToken->update(parentToken->state);
*/

void StateMachine::createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  nonInterruptingEventSubProcesses.push_back( std::make_unique<StateMachine>(pendingEventSubProcess) );
  nonInterruptingEventSubProcesses.back()->run(status);
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
//std::cerr << "start shutdown: " << scope->id << std::endl;
  auto& [key,completedTokens] = *it;

/*
  if ( !parentToken ) {
    // remove instance from system state
    const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(it);
    const_cast<SystemState*>(systemState)->completedStateMachines.push_back(this);

std::cerr << "shutdown (no parent): " << scope->id << std::endl;
    return;
  }
*/

  if ( parentToken ) {
    // merge tokens
    for ( auto& value : parentToken->status ) {
      value = std::nullopt;
    }

    for ( auto completedToken : completedTokens ) {
      parentToken->mergeStatus(completedToken);
      erase_ptr<Token>(tokens,completedToken);
    }
  }

  const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(it);

  const_cast<SystemState*>(systemState)->completedStateMachines.push_back(this);

//std::cerr << "shutdown (done): " << scope->id << "/" << systemState->completedStateMachines.size() <<std::endl;
}

void StateMachine::terminate() {
// TODO: resource activity must send error message
// TODO: request, release must send revoke message

  for ( auto& subProcess : subProcesses ) {
    subProcess->terminate();
  }
  subProcesses.clear();

  for ( auto& eventSubProcess : nonInterruptingEventSubProcesses ) {
    eventSubProcess->terminate();
  }
  nonInterruptingEventSubProcesses.clear();

  for ( auto& childToken : tokens ) {
    childToken->destroy();
  }
  tokens.clear();

  const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(this);
}

