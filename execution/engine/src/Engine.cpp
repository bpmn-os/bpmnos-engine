#include "Engine.h"
#include "Token.h"
#include "StateMachine.h"
#include "model/parser/src/extensionElements/ExtensionElements.h"
#include "model/parser/src/SequentialAdHocSubProcess.h"
#include "model/parser/src/DecisionTask.h"
#include "execution/utility/src/erase.h"
#include "execution/listener/src/Listener.h"
#include <cassert>

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
    if ( !status[Model::ExtensionElements::Index::Instance].has_value() ) {
      throw std::runtime_error("Engine: instance of process '" + process->id + "' has no id");
    }
    if ( !status[Model::ExtensionElements::Index::Timestamp].has_value() ) {
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
  if ( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto tokenAtSequentialPerformer = systemState->tokenAtSequentialPerformer.at(token);
    if ( tokenAtSequentialPerformer->performing ) {
      throw std::runtime_error("Engine: illegal start of sequential activity '" + token->node->id + "'" );
    }
    systemState->tokensAwaitingSequentialEntry.remove(token);
    // sequential performer is no longer idle
    tokenAtSequentialPerformer->performing = token;
    // use sequential performer status
    token->setStatus(tokenAtSequentialPerformer->status);
  }
  else {
    systemState->tokensAwaitingParallelEntry.remove(token);
  }

  // update token status
  if ( event.entryStatus.has_value() ) {
    token->status = event.entryStatus.value();
  }

  commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
}

void Engine::process(const CompletionEvent& event) {
//std::cerr << "CompletionEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  if ( token->node->represents<BPMNOS::Model::DecisionTask>() ) {
    systemState->tokensAwaitingChoice.remove(token);
  }
  else {
    systemState->tokensAwaitingTaskCompletion.remove(token);
  }
  // update token status
  token->status = event.updatedStatus;

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
}

void Engine::process(const ExitEvent& event) {
//std::cerr << "ExitEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event.token);
  systemState->tokensAwaitingExit.remove(token);

  if ( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    auto tokenAtSequentialPerformer = systemState->tokenAtSequentialPerformer.at(token);
    // update sequential performer status
    tokenAtSequentialPerformer->setStatus(token->status);
    tokenAtSequentialPerformer->update(tokenAtSequentialPerformer->state);
    // sequential performer becomes idle
    assert(tokenAtSequentialPerformer->performing);
    tokenAtSequentialPerformer->performing = nullptr;
    systemState->tokenAtSequentialPerformer.erase(token);
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
  if ( message->waitingToken ) {
    systemState->messageAwaitingDelivery.erase( message->waitingToken );
    commands.emplace_back(std::bind(&Token::advanceToCompleted,message->waitingToken), message->waitingToken);
  }
  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
}


void Engine::process(const ErrorEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  commands.emplace_back(std::bind(&Token::advanceToFailed,token), token);
}

void Engine::process([[maybe_unused]] const ClockTickEvent& event) {
//std::cerr << "ClockTickEvent " << std::endl;
  systemState->incrementTimeBy(clockTick);
  // trigger tokens awaiting timer
  while ( !systemState->tokensAwaitingTimer.empty() ) {
    auto it = systemState->tokensAwaitingTimer.begin();
    auto [time, token_ptr] = *it;
    if ( time > systemState->getTime() ) {
      break;
    }
    auto token = token_ptr.lock();
    assert( token );
    commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
    systemState->tokensAwaitingTimer.remove(token.get());
  }
}

void Engine::process([[maybe_unused]] const TerminationEvent& event) {
  throw std::runtime_error("Engine: TerminationEvent not yet implemented");
}


BPMNOS::number Engine::getCurrentTime() {
  return systemState->currentTime;
}

const SystemState* Engine::getSystemState() {
  return systemState.get();
}

