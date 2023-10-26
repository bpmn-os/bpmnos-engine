#include "Engine.h"
#include "execution/utility/src/erase.h"
#include "execution/listener/src/Listener.h"

using namespace BPMNOS::Execution;

Engine::Engine()
{
  clockTick = 1;
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
    if ( !systemState->isAlive() ) {
      break;
    }
    if ( systemState->getTime() > timeout ) {
      break;
    }
  }
}

bool Engine::advance() {
  // make sure new data is added to system state
  const_cast<BPMNOS::Model::Scenario*>(systemState->scenario)->update();

  // add all new instances and advance tokens as much as possible
  systemState->addInstances();
  // it is assumed that at least one clock tick or termination event is processed to ensure that 
  // each instance is only added once 

  // fetch and process all events
  while ( auto event = fetchEvent() ) {
    event->processBy(this);

    if ( event->is<ClockTickEvent>() ) {
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

void Engine::process(const ClockTickEvent& event) {
  systemState->incrementTimeBy(clockTick);
}

void Engine::process(const TaskCompletionEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  token->advanceToCompleted(event.updatedValues);
}

void Engine::process(const EntryEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  token->advanceToEntered(event.entryStatus);
}

void Engine::process(const ExitEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  token->advanceToExiting(event.exitStatus);
}

/*
void Engine::process(const MessageDeliveryEvent& event) {
  throw std::runtime_error("Engine: MessageDeliveryEvent not yet implemented");
}
*/

void Engine::process(const ReadyEvent& event) {
  Token* token = const_cast<Token*>(event.token);
  token->advanceToReady(event.values);
}

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

void Engine::advance(Token* token) {
  StateMachine* stateMachine = const_cast<StateMachine*>(token->owner);
  stateMachine->advance(*token);
}

void Engine::advance(const Token* token) {
  advance( const_cast<Token*>(token) );
}


