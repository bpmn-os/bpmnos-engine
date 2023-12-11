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
{
//std::cerr << "StateMachine(" << scope->id << ")" << this << std::endl;
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
{
//std::cerr << "cStateMachine(" << scope->id << ")" << this << ", parentToken: " << parentToken << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::StateMachine(const StateMachine* other)
  : systemState(other->systemState)
  , process(other->process)
  , scope(other->scope)
  , parentToken(other->parentToken)
{
//std::cerr << "oStateMachine(" << scope->id << ")" << this << ", parentToken: " << parentToken << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::~StateMachine() {
//std::cerr << "~StateMachine(" << scope->id << ")" << std::endl;
  const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.erase(this);
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(this);
}

void StateMachine::initiateBoundaryEvents(Token* token) {
//std::cerr << "initiateBoundaryEvents" << std::endl;
  auto activity = token->node->as<BPMN::Activity>();
  for ( auto boundaryEvent : activity->boundaryEvents ) {
    throw std::runtime_error("StateMachine: boundary events not yet implemented!");
  }
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

void StateMachine::run(const Values& status) {
//std::cerr << "Run " << scope->id << std::endl;
  if ( !parentToken ) {
    // state machine without parent token represents a token at a process
//std::cerr << "Start process " << process->id << std::endl;
    tokens.push_back( std::make_shared<Token>(this,nullptr,status) );
  }
  else {
    if ( scope->startNodes.size() != 1 ) {
      throw std::runtime_error("StateMachine: no unique start node within scope of '" + scope->id + "'");
    }

    tokens.push_back( std::make_shared<Token>(this,scope->startNodes.front(),status) );
  }

  auto token = tokens.back().get();
//std::cerr << ">" << token->jsonify().dump() << "<" << std::endl;
  if ( token->node && token->node->represents<BPMN::Activity>() ) {
    throw std::runtime_error("StateMachine: start node within scope of '" + scope->id + "' is an activity");
  }

  if ( token->node && token->node->represents<BPMN::CatchEvent>() && !token->node->represents<BPMN::UntypedStartEvent>() ) {
    bool isInterrupting = token->node->get<XML::bpmn::tStartEvent>()->isInterrupting->get().value;
    if ( !isInterrupting ) {
      // token instantiates non-interrupting event subprocess
      // get instantiation counter from context
      auto context = const_cast<StateMachine*>(parentToken->owned);
      auto counter = ++context->instantiations[token->node];
      // append instantiation counter for disambiguation
      std::string id = BPMNOS::to_string(token->status[Model::Status::Index::Instance].value(),STRING) + delimiter +  std::to_string(counter);
      token->status[Model::Status::Index::Instance] = BPMNOS::to_number(id,BPMNOS::ValueType::STRING);
    }
  }

  // advance token
  token->advanceToEntered();
}

void StateMachine::createChild(Token* parent, const BPMN::Scope* scope) {
  if ( scope->startNodes.size() > 1 ) {
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
    auto token = tokenCopies[i];
    auto engine = const_cast<Engine*>(systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,token,sequenceFlows[i]), weak_from_this(), token->weak_from_this());
  }
}

void StateMachine::createMergedToken(std::map< const BPMN::FlowNode*, std::vector<Token*> >::iterator gatewayIt) {
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
  engine->commands.emplace_back(std::bind(&Token::advanceToEntered,token), weak_from_this(), token->weak_from_this());
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

  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& eventSubProcess) {
    auto startNode = eventSubProcess->scope->startNodes.front();
    return startNode->represents<BPMN::EscalationStartEvent>();
  });

  auto engine = const_cast<Engine*>(systemState->engine);

  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
//std::cerr << "found event-subprocess catching escalation:" << eventToken << "/" << eventToken->owner << std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), const_cast<StateMachine*>(eventToken->owner)->weak_from_this(), eventToken->weak_from_this());

    return;
  }

//std::cerr << "bubbble up escalation" << std::endl;
  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,parent,parentToken), parent->weak_from_this(), parentToken->weak_from_this());

}


void StateMachine::handleFailure(Token* token) {
// TODO
//std::cerr << "handleFailure at " << (token->node ? token->node->id : process->id ) <<std::endl;

  if ( !parentToken ) {
//std::cerr << "process has failed" << std::endl;
    // process has failed
    pendingEventSubProcesses.clear();
    interruptingEventSubProcess.reset();
    nonInterruptingEventSubProcesses.clear();
    tokens.clear();

    return;
  }

  // update status of parent token with that of current token
  parentToken->status = token->status;

//std::cerr << "find event-subprocess catching error " << scope->id << "/" << parentToken << ": " << pendingEventSubProcesses.size() << std::endl;
  // find event-subprocess catching error
  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& eventSubProcess) {
    auto startNode = eventSubProcess->scope->startNodes.front();
//std::cerr << "start node " << startNode->id << std::endl;
    return startNode->represents<BPMN::ErrorStartEvent>();
  });

  auto engine = const_cast<Engine*>(systemState->engine);

  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
//std::cerr << "found event-subprocess catching error:" << eventToken << "/" << eventToken->owner << std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), const_cast<StateMachine*>(eventToken->owner)->weak_from_this(), eventToken->weak_from_this());

    return;
  }

//std::cerr << "bubbble up error" << std::endl;
  engine->commands.emplace_back(std::bind(&Token::advanceToFailed,parentToken), const_cast<StateMachine*>(parentToken->owner)->weak_from_this(), parentToken->weak_from_this());

}

void StateMachine::attemptGatewayActivation(const BPMN::FlowNode* node) {
// TODO: check
  if ( node->represents<BPMN::ParallelGateway>() ) {

    auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].find(node);
    auto& [key,arrivedTokens] = *gatewayIt;
    if ( arrivedTokens.size() == node->incoming.size() ) {
      // create merged token and advance it
      auto engine = const_cast<Engine*>(systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::createMergedToken,this,gatewayIt), weak_from_this());
    }
  }
  else {
    // TODO: determine sequence flows that have a token
    throw std::runtime_error("StateMachine: converging gateway type not yet supported");
  }
}

void StateMachine::shutdown(std::unordered_map<const StateMachine*, std::vector<Token*> >::iterator it) {
//std::cerr << "start shutdown: " << scope->id << "/" << const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.size() << std::endl;
 
  auto engine = const_cast<Engine*>(systemState->engine);

  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>(); 
    eventSubProcess && eventSubProcess->isNonInterrupting()
  ) {
    auto context = const_cast<StateMachine*>(parentToken->owned);
    engine->commands.emplace_back(std::bind(&StateMachine::deleteNonInterruptingEventSubProcess,context,this), weak_from_this());
    return;
  }


  auto& [key,completedTokens] = *it;

  if ( parentToken ) {
    // merge tokens
    for ( auto& value : parentToken->status ) {
      value = std::nullopt;
    }

    for ( auto completedToken : completedTokens ) {
      parentToken->mergeStatus(completedToken);
    }
  }

  if ( !parentToken ) {
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), weak_from_this());
  }
  else {
    auto parent = const_cast<StateMachine*>(parentToken->owner);
    auto context = const_cast<StateMachine*>(parentToken->owned);
    engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,context), context->weak_from_this());
//    engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), weak_from_this());
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

  // TODO
  auto it = const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.find(this);
  if ( it == const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.end() ) {
//std::cerr << const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.size() << std::endl;
//    throw std::logic_error("StateMachine: cannot find tokens awaiting state machine completion");
    return;
  }
//std::cerr << const_cast<SystemState*>(systemState)->tokensAwaitingStateMachineCompletion.size() << std::endl;

  auto& [key,completedTokens] = *it;

  if ( completedTokens.size() < tokens.size() ) {
    return;
  }
  else if ( completedTokens.size() == tokens.size() ) {
    // all tokens are in DONE state
    // no new event subprocesses can be triggered
    pendingEventSubProcesses.clear();
  }
  else {
    throw std::logic_error("StateMachine: too many tokens");
  }

  if ( nonInterruptingEventSubProcesses.size() ) {
    // wait until last event subprocess is completed
    return;
  }

  auto engine = const_cast<Engine*>(systemState->engine);
//std::cerr << "Shutdown with " << engine->commands.size()-1 << " prior commands" << std::endl;
  engine->commands.emplace_back(std::bind(&StateMachine::shutdown,this,it), weak_from_this());
}

void StateMachine::deleteChild(StateMachine* child) {
//std::cerr << "deleteChild" << std::endl;
  // state machine represents a completed (sub)process
  auto token = child->parentToken;
  erase_ptr<StateMachine>(subProcesses, child);
  auto engine = const_cast<Engine*>(systemState->engine);
  engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token), weak_from_this(), token->weak_from_this());
}

void StateMachine::deleteNonInterruptingEventSubProcess(StateMachine* eventSubProcess) {
//std::cerr << "deleteNonInterruptingEventSubProcess" << std::endl;
  erase_ptr<StateMachine>(nonInterruptingEventSubProcesses, eventSubProcess);
  attemptShutdown();
}
