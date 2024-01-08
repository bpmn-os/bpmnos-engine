#include "Engine.h"
#include "StateMachine.h"
#include "Token.h"
#include "SystemState.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "execution/utility/src/erase.h"
#include "model/parser/src/extensionElements/Status.h"
#include "model/parser/src/extensionElements/Timer.h"
#include "bpmn++.h"

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Process* process)
  : systemState(systemState)
  , process(process)
  , scope(process)
  , parentToken(nullptr)
{
//std::cerr << "StateMachine(" << scope->id  << "/" << this << " @ " << parentToken << ")" << std::endl;
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
{
//std::cerr << "cStateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")" << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::StateMachine(const StateMachine* other)
  : systemState(other->systemState)
  , process(other->process)
  , scope(other->scope)
  , parentToken(other->parentToken)
{
//std::cerr << "oStateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")"  << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::~StateMachine() {
//std::cerr << "~StateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")" << std::endl;
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(this);
  unregisterRecipient();
}

void StateMachine::initiateBoundaryEvents(Token* token) {
//std::cerr << "initiateBoundaryEvents" << std::endl;
  auto activity = token->node->as<BPMN::Activity>();
  for ( auto node : activity->boundaryEvents ) {
    initiateBoundaryEvent(token,node);
  }
}

void StateMachine::initiateBoundaryEvent(Token* token, const BPMN::FlowNode* node) {
//  throw std::runtime_error("StateMachine: boundary events not yet implemented!");
  tokens.push_back( std::make_shared<Token>(this,node,token->status) );
  auto createdToken = tokens.back().get();
  const_cast<SystemState*>(systemState)->tokenAtAssociatedActivity[createdToken] = token;
  const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token].push_back(createdToken);
  createdToken->advanceToEntered();
}

void StateMachine::initiateEventSubprocesses(Token* token) {
//std::cerr << "initiateEventSubprocesses for token at " << (token->node ? token->node->id : process->id ) << "/" << parentToken << "/" << token << " owned by " << token->owner << std::endl;
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    pendingEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, eventSubProcess, parentToken));
    auto pendingEventSubProcess = pendingEventSubProcesses.back().get();
//std::cerr << "Pending event subprocess has parent: " << pendingEventSubProcess->parentToken->jsonify().dump() << std::endl;

    pendingEventSubProcess->run(token->status);
  }
}

void StateMachine::registerRecipient() {
  if ( auto it = const_cast<SystemState*>(systemState)->unsent.find(instanceId);
    it != const_cast<SystemState*>(systemState)->unsent.end()
  ) {
    for ( auto& [message_ptr] : it->second ) {
      if ( auto message = message_ptr.lock(); message ) {
        const_cast<SystemState*>(systemState)->correspondence[this].emplace_back(message->weak_from_this());
        const_cast<SystemState*>(systemState)->outbox[message->origin].emplace_back(message->weak_from_this());
      }
    }
    const_cast<SystemState*>(systemState)->unsent.erase(it);
  }
}

void StateMachine::unregisterRecipient() {
  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>();
    !parentToken ||
    ( eventSubProcess && !eventSubProcess->startEvent->isInterrupting )
  ) {
    // delete all messages directed to state machine
    if ( auto it = const_cast<SystemState*>(systemState)->correspondence.find(this);
      it != const_cast<SystemState*>(systemState)->correspondence.end()
    ) {
      for ( auto& [message_ptr] : it->second ) {
        if ( auto message = message_ptr.lock(); message ) {
          erase_ptr(const_cast<SystemState*>(systemState)->messages, message.get());
        }
      }
      const_cast<SystemState*>(systemState)->correspondence.erase(it);
    }
  }
}

void StateMachine::run(const Values& status) {
//std::cerr << "Run " << scope->id << std::endl;
  if ( !parentToken ) {
    // state machine without parent token represents a token at a process
//std::cerr << "Start process " << process->id << std::endl;
    tokens.push_back( std::make_shared<Token>(this,nullptr,status) );
    const_cast<std::string&>(instanceId) = BPMNOS::to_string(status[Model::Status::Index::Instance].value(),STRING);
    const_cast<SystemState*>(systemState)->archive[ instanceId ] = weak_from_this();
    registerRecipient();
  }
  else {
    if ( scope->startEvents.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
    }

    tokens.push_back( std::make_shared<Token>(this,scope->startEvents.front(),status) );
  }

  auto token = tokens.back().get();
//std::cerr << ">" << token->jsonify().dump() << "<" << std::endl;
  if ( token->node && token->node->represents<BPMN::Activity>() ) {
    throw std::runtime_error("StateMachine: start node within scope of '" + scope->id + "' is an activity");
  }

  if ( token->node ) {
    if ( auto startEvent = token->node->represents<BPMN::TypedStartEvent>();
      startEvent && !startEvent->isInterrupting
    ) {
      // token instantiates non-interrupting event subprocess
      // get instantiation counter from context
      auto context = const_cast<StateMachine*>(parentToken->owned);
      auto counter = ++context->instantiations[token->node];
      // append instantiation counter for disambiguation
      const_cast<std::string&>(instanceId) = BPMNOS::to_string(token->status[Model::Status::Index::Instance].value(),STRING) + delimiter +  std::to_string(counter);
      token->status[Model::Status::Index::Instance] = BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING);
      const_cast<SystemState*>(systemState)->archive[ instanceId ] = weak_from_this();
      registerRecipient();
    }
  }

  // advance token
  token->advanceToEntered();
}

void StateMachine::createChild(Token* parent, const BPMN::Scope* scope) {
  if ( scope->startEvents.size() > 1 ) {
    throw std::runtime_error("StateMachine: scope '" + scope->id + "' has multiple start nodes");
  }

  subProcesses.push_back(std::make_shared<StateMachine>(systemState, scope, parent));
  auto subProcess = subProcesses.back().get();
  parent->owned = subProcess;
  subProcess->run(parent->status);
}

void StateMachine::createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  interruptingEventSubProcess = std::make_shared<StateMachine>(pendingEventSubProcess);
  interruptingEventSubProcess->run(status);
}

void StateMachine::createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  nonInterruptingEventSubProcesses.push_back( std::make_shared<StateMachine>(pendingEventSubProcess) );
  nonInterruptingEventSubProcesses.back()->run(status);
}

void StateMachine::createCompensationActivity(const BPMN::Activity* compensationActivity, const BPMNOS::Values& status) {
  if ( compensationActivity->represents<BPMN::Task>() ) {
    // create token at compensation activity
    std::shared_ptr<Token> compensationToken = std::make_shared<Token>(this,compensationActivity,status);
    compensationTokens.push_back(std::move(compensationToken));
  }
  else if ( auto compensationSubProcess = compensationActivity->represents<BPMN::SubProcess>(); compensationSubProcess ) {
    // create token that will own the compensation activity
    std::shared_ptr<Token> compensationToken = std::make_shared<Token>(this,compensationSubProcess,status);
    // create state machine for compensation activity
    compensations.push_back(std::make_shared<StateMachine>(systemState, compensationSubProcess, compensationToken.get()));
    compensationToken->owned = compensations.back().get();
    compensationTokens.push_back(std::move(compensationToken));
  }
}

void StateMachine::createCompensationEventSubProcess(const BPMN::EventSubProcess* compensationEventSubProcess, const BPMNOS::Values& status) {
  // create state machine for compensation event subprocess
  compensations.push_back(std::make_shared<StateMachine>(systemState, compensationEventSubProcess, parentToken));
  // create token at start event of compensation event subprocess
  std::shared_ptr<Token> compensationToken = std::make_shared<Token>(compensations.back().get(), compensationEventSubProcess->startEvent, status );
  compensationTokens.push_back(std::move(compensationToken));
}

void StateMachine::createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows) {
  std::vector<Token*> tokenCopies;
  // create a token copy for each new destination
  for ( [[maybe_unused]] auto _ : sequenceFlows ) {
    tokens.push_back( std::make_shared<Token>( token ) );
    tokenCopies.push_back(tokens.back().get());
  }

  // remove original token
  erase_ptr<Token>(tokens,token);

  // advance all token copies
  for (size_t i = 0; i < sequenceFlows.size(); i++ ) {
    auto tokenCopy = tokenCopies[i];
    auto engine = const_cast<Engine*>(systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,tokenCopy,sequenceFlows[i]), tokenCopy);
  }
}

void StateMachine::createMergedToken(const BPMN::FlowNode* gateway) {
  auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].find(gateway);
  auto& [key,arrivedTokens] = *gatewayIt;

  // create merged token
  std::shared_ptr<Token> mergedToken = std::make_shared<Token>(arrivedTokens);

  // remove tokens
  for ( auto arrivedToken : arrivedTokens ) {
    erase_ptr<Token>(tokens,arrivedToken);
  }
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].erase(gatewayIt);

  // add merged token
  tokens.push_back(std::move(mergedToken));

  // advance merged token
  auto token = tokens.back().get();
  auto engine = const_cast<Engine*>(systemState->engine);
  engine->commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
}


void StateMachine::copyToken(Token* token) {
// TODO: check
  if ( token->node->represents<BPMN::ParallelGateway>() ) {
    // create token copies and advance them
    createTokenCopies(token, token->node->outgoing);
  }
  else {
    // TODO: determine sequence flows that receive a token
    throw std::runtime_error("StateMachine: diverging gateway type not yet supported");
  }
}

void StateMachine::handleEscalation(Token* token) {
// TODO
  if ( !parentToken ) {
    return;
  }

  // update status of parent token with that of current token
  parentToken->status = token->status;
  parentToken->update(parentToken->state);

  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& stateMachine) {
    auto eventSubProcess = stateMachine->scope->as<BPMN::EventSubProcess>();
    return eventSubProcess->startEvent->represents<BPMN::EscalationStartEvent>();
  });

  auto engine = const_cast<Engine*>(systemState->engine);

  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
//std::cerr << "found event-subprocess catching escalation:" << eventToken << "/" << eventToken->owner << std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);

    return;
  }

  // find escalation boundary event
  if ( parentToken->node ) {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[parentToken];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::EscalationBoundaryEvent>() ) {
        engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);
        return;
      }
    }
  }

//std::cerr << "bubbble up escalation" << std::endl;
  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,parent,parentToken), parentToken);

}


void StateMachine::handleFailure(Token* token) {
//std::cerr << scope->id << " handles failure at " << (token->node ? token->node->id : process->id ) <<std::endl;

  if ( !parentToken ) {
//std::cerr << "process has failed" << std::endl;
    // process has failed
    pendingEventSubProcesses.clear();
    interruptingEventSubProcess.reset();
    nonInterruptingEventSubProcesses.clear();
    tokens.clear();
    compensations.clear();
    return;
  }

  auto engine = const_cast<Engine*>(systemState->engine);

  // lambda definition for finding of token at error boundary event
  auto tokenAwaitingErrorBoundaryEvent = [this](Token* token) -> Token*
  {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(this->systemState)->tokensAwaitingBoundaryEvent[token];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::ErrorBoundaryEvent>() ) {
        return eventToken;
      }
    }
    return nullptr;
  };

  if ( token->node && token->node->represents<BPMN::Task>() ) {
    // find error boundary event
    if ( auto eventToken = tokenAwaitingErrorBoundaryEvent(token); eventToken ) {
      engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);
      return;
    }
    // failure is not caught by boundary event of task
  }

  // find event-subprocess catching error
  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& stateMachine) {
    auto eventSubProcess = stateMachine->scope->as<BPMN::EventSubProcess>();
    return eventSubProcess->startEvent->represents<BPMN::ErrorStartEvent>();
  });
  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
    // update status of event token with that of current token
    eventToken->status = token->status;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);

    return;
  }

  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), this);

  // failure is not caught by event subprocess

  // update status of parent token with that of current token
  parentToken->status = token->status; // TODO resize status // TODO use std::move?
  engine->commands.emplace_back(std::bind(&Token::update,parentToken,Token::State::FAILED), parentToken);
  // DO I NEED A STATE FAILING OR COMPENSATING???

  // conduct all compensations in reversed order
  for ( auto& compensationToken : compensationTokens | std::views::reverse ) {
    // TODO: wait for all compensations to be completed before continuing with below
    return;
  }

  // TODO: BELOW NEEDS TO BE PUT IN METHOD TO BE RESUMED FOR COMPENSATING/FAILING STATE MACHINES

  // find error boundary event
  if ( auto eventToken = tokenAwaitingErrorBoundaryEvent(parentToken); eventToken ) {
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);
    return;
  }

  // failure is not caught by boundary event, bubble up error
//std::cerr << "bubbble up error" << std::endl;
  engine->commands.emplace_back(std::bind(&Token::advanceToFailed,parentToken), parentToken);
}

void StateMachine::attemptGatewayActivation(const BPMN::FlowNode* node) {
// TODO: check
  if ( node->represents<BPMN::ParallelGateway>() ) {

    auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].find(node);
    auto& [key,arrivedTokens] = *gatewayIt;
    if ( arrivedTokens.size() == node->incoming.size() ) {
      // create merged token and advance it
      auto engine = const_cast<Engine*>(systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::createMergedToken,this,node), this);
    }
  }
  else {
    // TODO: determine sequence flows that have a token
    throw std::runtime_error("StateMachine: converging gateway type not yet supported");
  }
}

void StateMachine::shutdown() {
//std::cerr << "start shutdown: " << scope->id << std::endl;
  auto engine = const_cast<Engine*>(systemState->engine);

  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>();
    eventSubProcess && !eventSubProcess->startEvent->isInterrupting
  ) {
    if ( compensations.empty() ) {
      auto context = const_cast<StateMachine*>(parentToken->owned);
      engine->commands.emplace_back(std::bind(&StateMachine::deleteNonInterruptingEventSubProcess,context,this), this);
    }
    return;
  }

  // update status of parent token
  if ( parentToken ) {
    // merge tokens
    for ( auto& value : parentToken->status ) {
      value = std::nullopt;
    }

    for ( auto token : tokens ) {
      parentToken->mergeStatus(token.get());
    }
  }

  // ensure that messages to state machine are removed  
  unregisterRecipient();

  if ( auto activity = scope->represents<BPMN::Activity>(); activity && activity->compensatedBy ) {
    if ( auto compensationEventSubProcess = activity->compensatedBy->represents<BPMN::EventSubProcess>();
      compensationEventSubProcess ) {
      // set owner of compensations
      auto owner = const_cast<StateMachine*>(parentToken->owner);

      // create compensation event subprocess
        engine->commands.emplace_back( std::bind(&StateMachine::createCompensationEventSubProcess,owner,compensationEventSubProcess, parentToken->status), owner );
    }
  }

  if ( !parentToken ) {
    // delete root state machine (and all descendants)
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), this);
  }
  else {
    if ( scope->represents<BPMN::SubProcess>() && compensations.empty() ) {
      auto parent = const_cast<StateMachine*>(parentToken->owner);
      engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), this);
    }

    // advance parent token to completed
    auto context = const_cast<StateMachine*>(parentToken->owned);
    auto token = context->parentToken;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
  }

//std::cerr << "shutdown (done): " << scope->id <<std::endl;
}

void StateMachine::attemptShutdown() {
// TODO: check
//std::cerr << "attemptShutdown: " << scope->id << "/" << this << std::endl;
  if ( interruptingEventSubProcess ) {
//std::cerr << "wait for completion of interrupting event subprocess" << interruptingEventSubProcess->scope->id << std::endl;
    // wait for completion of interrupting event subprocess
    return;
  }

  if ( nonInterruptingEventSubProcesses.size() ) {
    // wait until last event subprocess is completed
    return;
  }

  for ( auto& token : tokens ) {
    if ( token->state != Token::State::DONE ) {
      return;
    }
  }

  // all tokens are in DONE state
  // no new event subprocesses can be triggered
  pendingEventSubProcesses.clear();

  auto engine = const_cast<Engine*>(systemState->engine);
//std::cerr << "Shutdown with " << engine->commands.size()-1 << " prior commands" << std::endl;
  engine->commands.emplace_back(std::bind(&StateMachine::shutdown,this), this);
}

Token* StateMachine::findCompensationToken(BPMN::Node* compensationNode) {
  Tokens::iterator it;
  if ( auto compensationActivity = compensationNode->represents<BPMN::Activity>();
    compensationActivity
  ) {
     it = std::find_if(
       compensationTokens.begin(),
       compensationTokens.end(),
       [&compensationActivity](const auto& token) {
         // check if compensation token is at compensation activity
         return ( token->node == compensationActivity );
       }
    );
  }
  else if ( auto compensationEventSubProcess = compensationNode->represents<BPMN::EventSubProcess>();
    compensationEventSubProcess
  ) {
    it = std::find_if(
      compensationTokens.begin(),
      compensationTokens.end(),
      [&compensationEventSubProcess](const auto& token) {
         // check if compensation token is at node (i.e. start event) of compensation event subprocess
         return ( token->owner->scope == compensationEventSubProcess );
      }
    );
  }
  else {
    throw std::logic_error("StateMachine: illegal compensation");
  }

  if ( it != compensationTokens.end() ) {
    return it->get();
  }

  return nullptr;
}


void StateMachine::deleteChild(StateMachine* child) {
//std::cerr << "delete child '" << child->scope->id << "' of '" << scope->id << "'" <<  std::endl;
  erase_ptr<StateMachine>(subProcesses, child);
}

void StateMachine::deleteNonInterruptingEventSubProcess(StateMachine* eventSubProcess) {
//std::cerr << "deleteNonInterruptingEventSubProcess" << std::endl;
  erase_ptr<StateMachine>(nonInterruptingEventSubProcesses, eventSubProcess);
  attemptShutdown();
}

void StateMachine::interruptActivity(Token* token) {
//std::cerr << "interrupt activity " << token->node->id << std::endl;
  deleteTokensAwaitingBoundaryEvent(token);
  erase_ptr<Token>(tokens, token);
}

void StateMachine::deleteTokensAwaitingBoundaryEvent(Token* token) {
  // delete all tokens awaiting boundary event
  auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent;
  auto it = tokensAwaitingBoundaryEvent.find(token);
  if ( it != tokensAwaitingBoundaryEvent.end() ) {
    for ( auto waitingToken : it->second ) {
      erase_ptr<Token>(tokens, waitingToken);
    }
    tokensAwaitingBoundaryEvent.erase(it);
  }
}

