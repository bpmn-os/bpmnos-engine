#include "Engine.h"
#include "Token.h"
#include "StateMachine.h"
#include "execution/utility/src/erase.h"
#include "execution/listener/src/Listener.h"

using namespace BPMNOS::Execution;

Engine::Engine()
{
  clockTick = 1;
}

Engine::~Engine()
{
}

void Engine::Command::execute() {
  if ( token_ptr.has_value() ) {
    auto shared_token_ptr = token_ptr->lock();
    if ( !shared_token_ptr ) {
      // relevant token has expired, skip command
      return;
    }
  }

  function();
}

void Engine::addEventHandler(EventHandler* eventHandler) {
  eventHandlers.push_back(eventHandler);
}

std::unique_ptr<Event> Engine::fetchEvent() {
  for ( auto eventHandler : eventHandlers ) {
    if ( auto event = eventHandler->fetchEvent(systemState.get()) ) {
      return event;
    }
  }
  return nullptr;
}

void Engine::addListener(Listener* listener) {
  listeners.push_back(listener);
}

void Engine::run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number timeout) {
  // create initial system state
  systemState = std::make_unique<SystemState>(this, scenario);

  // advance all tokens in system state
  while ( advance() ) {
//std::cerr << ".";
    if ( !systemState->isAlive() ) {
//std::cerr << "dead" << std::endl;
      break;
    }
    if ( systemState->getTime() > timeout ) {
//std::cerr << "timeout" << std::endl;
      break;
    }
  }
}

bool Engine::advance() { 
  // make sure new data is added to system state
  const_cast<BPMNOS::Model::Scenario*>(systemState->scenario)->update();

  // add all new instances and advance tokens
  addInstances();
  // it is assumed that at least one clock tick or termination event is processed to ensure that 
  // each instance is only added once 

  while ( commands.size() ) {
    commands.front().execute();
    commands.pop_front();
  }

  // fetch and process all events
  while ( auto event = fetchEvent() ) {
//std::cerr << "*";
    event->processBy(this);

    while ( commands.size() ) {
      commands.front().execute();
      commands.pop_front();
    }

    if ( event->is<ClockTickEvent>() ) {
//std::cerr << "ClockTick" << std::endl;
      // exit loop to resume 
      return true;
    }

    if ( event->is<TerminationEvent>() ) {
      // exit loop to terminate
      return false;
    }
  }

  throw std::runtime_error("Engine: unexpected absence of event");
}

void Engine::addInstances() {
  for (auto& [process,status] : systemState->getInstantiations() ) {
    systemState->instantiationCounter++;
    systemState->instances.push_back(std::make_unique<StateMachine>(systemState.get(),process));
    // run instance and advance token
    systemState->instances.back()->run(status);
  }
}

void Engine::deleteInstance(StateMachine* instance) {
//std::cerr << "deleteInstance" << std::endl;
  erase_ptr<StateMachine>(systemState->instances,instance);
}

void Engine::process(const ClockTickEvent& event) {
//std::cerr << "ClockTickEvent " << std::endl;
  systemState->incrementTimeBy(clockTick);
}

void Engine::process(const ReadyEvent& event) {
//std::cerr << "ReadyEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingReadyEvent.remove(token);

  token->sequenceFlow = nullptr;
  token->status.insert(token->status.end(), event.values.begin(), event.values.end());

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  commands.emplace_back(std::bind(&Token::advanceToReady,token), stateMachine, token->weak_from_this());
}

void Engine::process(const EntryEvent& event) {
//std::cerr << "EntryEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  if ( auto tokenAtResource = event.token->getResourceToken(); tokenAtResource ) {
    systemState->tokensAwaitingJobEntryEvent[tokenAtResource].remove(token);
    // resource is no longer idle
    systemState->tokensAtIdleResources.remove(tokenAtResource);
  }
  else {
    systemState->tokensAwaitingRegularEntryEvent.remove(token);
  }

  // update token status
  if ( event.entryStatus.has_value() ) {
    token->status = event.entryStatus.value();
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  commands.emplace_back(std::bind(&Token::advanceToEntered,token), stateMachine, token->weak_from_this());
}

void Engine::process(const TaskCompletionEvent& event) {
//std::cerr << "TaskCompletionEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingTaskCompletionEvent.remove(token);

  // update token status
  for ( auto & [index, value] : event.updatedValues ) {
    token->status[index] = value;
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), stateMachine, token->weak_from_this());
}

void Engine::process(const ExitEvent& event) {
//std::cerr << "ExitEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingExitEvent.remove(token);

  if ( auto tokenAtResource = token->getResourceToken(); tokenAtResource ) {
    // resource becomes idle
    systemState->tokensAtIdleResources.push_back( tokenAtResource->weak_from_this() );
  }

  // update token status
  if ( event.exitStatus.has_value() ) {
    token->status = event.exitStatus.value();
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(token->owner);
  commands.emplace_back(std::bind(&Token::advanceToExiting,token), stateMachine, token->weak_from_this());
}

/*
void Engine::process(const MessageDeliveryEvent& event) {
  throw std::runtime_error("Engine: MessageDeliveryEvent not yet implemented");
}
*/


void Engine::process(const TerminationEvent& event) {
  throw std::runtime_error("Engine: TerminationEvent not yet implemented");
}

/*
void Engine::process(const TimerEvent& event) {
  throw std::runtime_error("Engine: TimerEvent not yet implemented");
}
*/

BPMNOS::number Engine::getCurrentTime() {
  return systemState->currentTime;
}

const SystemState* Engine::getSystemState() {
  return systemState.get();
}

