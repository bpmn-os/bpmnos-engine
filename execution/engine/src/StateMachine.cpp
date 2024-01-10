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
    if ( !node->represents<BPMN::CompensateBoundaryEvent>() ) {
      initiateBoundaryEvent(token,node);
    }
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
//std::cerr << "initiate " << scope->eventSubProcesses.size() << " eventSubprocesses for token at " << (token->node ? token->node->id : process->id ) << "/" << parentToken << "/" << token << " owned by " << token->owner << std::endl;
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
    // state machine without parent token represents a process
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

//std::cerr << "Initial token: >" << token->jsonify().dump() << "<" << std::endl;
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

void StateMachine::createCompensationTokenForBoundaryEvent(const BPMN::BoundaryEvent* compensateBoundaryEvent, Token* token) {
//std::cerr << "createCompensationTokenForBoundaryEvent" <<std::endl;
  std::shared_ptr<Token> compensationToken = std::make_shared<Token>(this,compensateBoundaryEvent,token->status);
  compensationToken->update(Token::State::BUSY);
  compensationTokens.push_back(std::move(compensationToken));
}

void StateMachine::compensateActivity(Token* token) {
//std::cerr << "compensateActivity" <<std::endl;
  auto compensationNode = token->node->as<BPMN::BoundaryEvent>()->attachedTo->compensatedBy;
  if ( auto compensationActivity = compensationNode->represents<BPMN::Activity>() ) {
    // move token to compensation activity
    token->node = compensationActivity;
    auto engine = const_cast<Engine*>(systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
  }
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

void StateMachine::terminate(Token* token) {
//std::cerr << "terminate" <<std::endl;
  auto engine = const_cast<Engine*>(systemState->engine);

//  auto parent = const_cast<StateMachine*>(parentToken->owner);
//  engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), this);


  // find error boundary event
  if ( auto eventToken = findTokenAwaitingErrorBoundaryEvent(token); eventToken ) {
    engine->commands.emplace_back(std::bind(&Token::update,token,Token::State::FAILED), token);
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);
    return;
  }

  // failure is not caught by boundary event, bubble up error
  engine->commands.emplace_back(std::bind(&Token::advanceToFailed,token), token);
}


void StateMachine::handleFailure(Token* token) {
  auto engine = const_cast<Engine*>(systemState->engine);

//std::cerr << scope->id << " handles failure at " << (token->node ? token->node->id : process->id ) <<std::endl;
  if ( !parentToken ) {
//std::cerr << "process has failed" << std::endl;
    // process has failed
    pendingEventSubProcesses.clear();
    interruptingEventSubProcess.reset();
    nonInterruptingEventSubProcesses.clear();
    tokens.clear();
    compensationTokens.clear();
    // delete root state machine (and all descendants)
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), this);
    return;
  }

//std::cerr << "check whether failure is caught" << std::endl;

  if ( token->node && token->node->represents<BPMN::Task>() ) {
    // find error boundary event
    if ( auto eventToken = findTokenAwaitingErrorBoundaryEvent(token); eventToken ) {
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


  // failure is not caught by event subprocess
//std::cerr << "failure is not caught by event subprocess" << std::endl;

  // update status of parent token with that of current token
  parentToken->status = token->status; // TODO resize status 
  engine->commands.emplace_back(std::bind(&Token::update,parentToken,Token::State::FAILING), parentToken);

  if ( auto compensations = getCompensationTokens(); compensations.size() ) {
    // wait for all compensations to be completed before continuing with below
//std::cerr << "Start compensation ... " << compensations.size() << std::endl;
    compensate(compensations, parentToken);
    return;
  }

  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), this);
  terminate(parentToken);
}

Token* StateMachine::findTokenAwaitingErrorBoundaryEvent(Token* activityToken) {
  auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(this->systemState)->tokensAwaitingBoundaryEvent[activityToken];
  for ( auto eventToken : tokensAwaitingBoundaryEvent) {
    if ( eventToken->node->represents<BPMN::ErrorBoundaryEvent>() ) {
      return eventToken;
    }
  }
  return nullptr;
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
    auto context = const_cast<StateMachine*>(parentToken->owned);
    engine->commands.emplace_back(std::bind(&StateMachine::deleteNonInterruptingEventSubProcess,context,this), this);
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

  if ( !parentToken ) {
    // delete root state machine (and all descendants)
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), this);
  }
  else {
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

std::vector<Token*> StateMachine::getCompensationTokens(const BPMN::Activity* activity) const {
//std::cerr << "getCompensationTokens of " << ( activity ? activity->id : std::string("all activities") ) << " compensated by " <<  ( activity ? activity->compensatedBy->id : std::string("n/a") ) << std::endl;
  std::vector<Token*> result;
  if ( compensationTokens.empty() ) {
//std::cerr << "no compensation token" << std::endl;
    return result;
  }

  if ( activity ) {
    // find compensation within context
    auto it = std::find_if(
       compensationTokens.begin(),
       compensationTokens.end(),
       [&activity](const std::shared_ptr<Token>& token) -> bool {
         // check if compensation token is at compensation boundary event
         return ( token.get()->node->as<BPMN::BoundaryEvent>()->attachedTo == activity );
       }
    );
    if ( it != compensationTokens.end() ) {
      result.push_back(it->get());
    }
  }
  else {
    result.reserve(compensationTokens.size());
    for ( auto compensationToken : compensationTokens ) {
      result.push_back(compensationToken.get());
    }
  }

  return result;
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

void StateMachine::completeCompensationActivity(Token* token) {
//std::cerr << "Compensation completed: " << token->node->id << std::endl;
  // token is still the compensation token
  auto engine = const_cast<Engine*>(systemState->engine);

  // advance waiting token
  auto& tokenAwaitingCompletedCompensation = const_cast<SystemState*>(systemState)->tokenAwaitingCompletedCompensation;
  auto waitingToken = tokenAwaitingCompletedCompensation[token];
  if ( waitingToken->state == Token::State::BUSY ) {
//std::cerr << "Continue with advanceToCompleted: " << waitingToken->node->id << std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,waitingToken), waitingToken);
  }
  else if ( waitingToken->state == Token::State::FAILING ) {
//std::cerr << "Continue with terminate: " << waitingToken->owner->scope->id << std::endl;
    engine->commands.emplace_back(std::bind(&StateMachine::terminate,const_cast<StateMachine*>(waitingToken->owner),waitingToken), waitingToken);
  }
  else {
//std::cerr << waitingToken->node->id << " has state: " << Token::stateName[(int)waitingToken->state]  << std::endl;
  }
  tokenAwaitingCompletedCompensation.erase(token);

  // remove compensation token
  erase_ptr<Token>(compensationTokens,token);
}

void StateMachine::compensate(std::vector<Token*> compensations, Token* waitingToken) {
  auto engine = const_cast<Engine*>(systemState->engine);
  auto& tokenAwaitingCompletedCompensation = const_cast<SystemState*>(systemState)->tokenAwaitingCompletedCompensation;

  auto it = compensations.rbegin();
  // advance last compensation token
  auto compensationToken = *it;
//std::cerr << engine->commands.size() << " > Compensate " << compensationToken->node->id << std::endl;
  engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,compensationToken), compensationToken);
  // got to prior compensation token
  it++;
  while ( it != compensations.rend() ) {
    // wait for completion of compensation
    tokenAwaitingCompletedCompensation[compensationToken] = *it;
    compensationToken = *it;
    // advance compensation token
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,compensationToken), compensationToken);
  }
  // wait for completion of compensation
  tokenAwaitingCompletedCompensation[compensationToken] = waitingToken;
}

