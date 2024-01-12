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
//std::cerr << "~Engine()" << std::endl;
}

void Engine::Command::execute() {
  if ( token_ptr.has_value() && token_ptr->expired() ) {
    // relevant token no longer exists, skip command
    return;
  }

  if ( stateMachine_ptr.has_value() && stateMachine_ptr->expired() ) {
    // relevant state machine no longer exists, skip command
    return;
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
//std::cerr << "execute" << std::endl;
    auto command = std::move(commands.front());
    commands.pop_front();
    command.execute();
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
    if ( !process->isExecutable ) {
      throw std::runtime_error("Engine: process is not executable");
    }
    if ( !status[Model::Status::Index::Instance].has_value() ) {
      throw std::runtime_error("Engine: instance of process '" + process->id + "' has no id");
    }
    if ( !status[Model::Status::Index::Timestamp].has_value() ) {
      throw std::runtime_error("Engine: instance of process '" + process->id + "' has no timestamp");
    }
    systemState->instantiationCounter++;
    systemState->instances.push_back(std::make_shared<StateMachine>(systemState.get(),process));
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

  commands.emplace_back(std::bind(&Token::advanceToReady,token), token);
}

void Engine::process(const EntryEvent& event) {
//std::cerr << "EntryEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  if ( auto tokenAtResource = event.token->getResourceToken() ) {
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

  commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
}

void Engine::process(const TaskCompletionEvent& event) {
//std::cerr << "TaskCompletionEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingTaskCompletionEvent.remove(token);

  // update token status
  token->status = event.updatedStatus;

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
}

void Engine::process(const ExitEvent& event) {
//std::cerr << "ExitEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingExitEvent.remove(token);

  if ( auto tokenAtResource = token->getResourceToken() ) {
    // resource becomes idle
    systemState->tokensAtIdleResources.emplace_back( tokenAtResource->weak_from_this() );
  }

  // update token status
  if ( event.exitStatus.has_value() ) {
    token->status = event.exitStatus.value();
  }

  commands.emplace_back(std::bind(&Token::advanceToExiting,token), token);
}

void Engine::process(const MessageDeliveryEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  Message* message = const_cast<Message*>(event.message);

  message->update(token);
  erase_ptr<Message>(systemState->messages,message);
  systemState->tokensAwaitingMessageDelivery.remove(token);

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
}


void Engine::process(const TerminationEvent& event) {
  throw std::runtime_error("Engine: TerminationEvent not yet implemented");
}

void Engine::process(const ErrorEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  commands.emplace_back(std::bind(&Token::advanceToFailed,token), token);
}

BPMNOS::number Engine::getCurrentTime() {
  return systemState->currentTime;
}

const SystemState* Engine::getSystemState() {
  return systemState.get();
}

