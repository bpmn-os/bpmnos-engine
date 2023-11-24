#include "Engine.h"
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

  // add all new instances and advance tokens as much as possible
  addInstances();
  // it is assumed that at least one clock tick or termination event is processed to ensure that 
  // each instance is only added once 

  clearCompletedStateMachines();

  // fetch and process all events
  while ( auto event = fetchEvent() ) {
//std::cerr << "*";
    event->processBy(this);

    while ( commands.size() ) {
      commands.front().execute();
      commands.pop_front();
    }

    clearCompletedStateMachines();

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

void Engine::clearCompletedStateMachines() { 
//std::cerr << "clearCompletedStateMachines " << systemState->completedStateMachines.size() << std::endl;
  // delete all completed child instances and advance parent token
  while ( systemState->completedStateMachines.size() ) {
    auto stateMachine = const_cast<StateMachine*>(systemState->completedStateMachines.back());
    systemState->completedStateMachines.pop_back();

    if ( !stateMachine->parentToken ) {
      deleteInstance(stateMachine);
    }
    else {
      if ( auto eventSubProcess = stateMachine->scope->represents<BPMN::EventSubProcess>(); eventSubProcess ) {
        if ( eventSubProcess->isInterrupting ) {
          auto parent = const_cast<StateMachine*>(stateMachine->parentToken->owner);
          parent->advanceToken(stateMachine->parentToken, Token::State::COMPLETED);
        }
        else {
          throw std::runtime_error("Engine: Non-interrupting event subprocesses not yet implemented");
        }
      }
      else {
        // state machine represents a completed (sub)process
        auto parent = const_cast<StateMachine*>(stateMachine->parentToken->owner);
        erase_ptr<StateMachine>(parent->subProcesses, stateMachine);
        parent->advanceToken(stateMachine->parentToken, Token::State::COMPLETED);
      }
    }

  }
}

void Engine::addInstances() {
  for (auto& [process,status] : systemState->getInstantiations() ) {
    systemState->instantiationCounter++;
    systemState->instances.push_back(std::make_unique<StateMachine>(systemState.get(),process));
    // advance token as far as possible
    systemState->instances.back()->run(status);
  }
}

void Engine::deleteInstance(StateMachine* instance) {
  erase_ptr<StateMachine>(systemState->instances,instance);
}

void Engine::process(const ClockTickEvent& event) {
//std::cerr << "ClockTickEvent " << std::endl;
  systemState->incrementTimeBy(clockTick);
}

void Engine::process(const ReadyEvent& event) {
  erase<Token*>(systemState->tokensAwaitingReadyEvent, event.token);

  Token* token = const_cast<Token*>(event.token);
//  token->advanceToReady(event.values); // TODO: update token status
  token->sequenceFlow = nullptr;
  token->status.insert(token->status.end(), event.values.begin(), event.values.end());

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  stateMachine->advanceToken(token, Token::State::READY);
}

void Engine::process(const EntryEvent& event) {
  if ( auto tokenAtResource = event.token->getResourceToken(); tokenAtResource ) {
    erase<Token*>(systemState->tokensAwaitingJobEntryEvent[tokenAtResource], event.token);
    // resource is no longer idle
    erase<Token*>(systemState->tokensAtIdleResources, tokenAtResource);
  }
  else {
    erase<Token*>(systemState->tokensAwaitingRegularEntryEvent, event.token);
  }

  Token* token = const_cast<Token*>(event.token);
//  token->advanceToEntered(event.entryStatus); // TODO: update token status
  if ( event.entryStatus.has_value() ) {
    token->status = event.entryStatus.value();
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  stateMachine->advanceToken(token, Token::State::ENTERED);
}

void Engine::process(const TaskCompletionEvent& event) {
//std::cerr << "TaskCompletionEvent " << event.token->node->id << "/" << systemState->tokensAwaitingTaskCompletionEvent.size() << std::endl;
  auto it = systemState->tokensAwaitingTaskCompletionEvent.begin();
  while (it != systemState->tokensAwaitingTaskCompletionEvent.end()) {
    if (it->second == event.token) {
        systemState->tokensAwaitingTaskCompletionEvent.erase(it);
        break; // Remove the first occurrence and exit the loop
    }
    ++it;
  }

  Token* token = const_cast<Token*>(event.token);
//  token->advanceToCompleted(event.updatedValues); // TODO: update token status
  for ( auto & [index, value] : event.updatedValues ) {
    token->status[index] = value;
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  stateMachine->advanceToken(token, Token::State::COMPLETED);
}

void Engine::process(const ExitEvent& event) {
  erase<Token*>(systemState->tokensAwaitingExitEvent, event.token);
  if ( auto tokenAtResource = event.token->getResourceToken(); tokenAtResource ) {
    // resource becomes idle
    systemState->tokensAtIdleResources.push_back(tokenAtResource);
  }

  Token* token = const_cast<Token*>(event.token);
//  token->advanceToExiting(event.exitStatus); // TODO: update token status
  if ( event.exitStatus.has_value() ) {
    token->status = event.exitStatus.value();
  }

  StateMachine* stateMachine = const_cast<StateMachine*>(event.token->owner);
  stateMachine->advanceToken(token, Token::State::EXITING);
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

