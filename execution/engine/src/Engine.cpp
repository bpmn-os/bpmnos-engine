#include "Engine.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/utility/src/erase.h"

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

void Engine::process(const ChoiceEvent& event) {
  throw std::runtime_error("Engine: ChoiceEvent not yet implemented");
}

void Engine::process(const ClockTickEvent& event) {
  systemState->incrementTimeBy(clockTick);
}

void Engine::process(const CompletionEvent& event) {
  throw std::runtime_error("Engine: CompletionEvent not yet implemented");
}

void Engine::process(const EntryEvent& event) {
  throw std::runtime_error("Engine: EntryEvent not yet implemented");
}

void Engine::process(const ExitEvent& event) {
  throw std::runtime_error("Engine: ExitEvent not yet implemented");
}

void Engine::process(const MessageDeliveryEvent& event) {
  throw std::runtime_error("Engine: MessageDeliveryEvent not yet implemented");
}

void Engine::process(const ReadyEvent& event) {
  Token* token = const_cast<Token*>(event.token);

  // remove token from awaitingReady
  systemState->awaitingReady.erase( std::find(systemState->awaitingReady.begin(), systemState->awaitingReady.end(), token) );

  // add new values to status
  token->status.insert(token->status.end(), event.values.begin(), event.values.end());

  // update token state
  token->update(Token::State::READY);

  // advance token as much as possible
  advance(token);
}

void Engine::process(const TerminationEvent& event) {
  throw std::runtime_error("Engine: TerminationEvent not yet implemented");
}
void Engine::process(const TriggerEvent& event) {
  throw std::runtime_error("Engine: TriggerEvent not yet implemented");
}

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


