#include "Engine.h"
#include "execution/engine/src/events/InstantiationEvent.h"
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

/*
std::unique_ptr<Event> Engine::listen( const SystemState& systemState ) {
  for ( auto eventHandler : eventHandlers ) {
    if ( auto event = eventHandler->fetchEvent(systemState) ) {
      return event;
    }
  }
  return nullptr;
}
*/

void Engine::run(const BPMNOS::Model::Scenario* scenario) {
  // create initial system state
  // TODO
  throw std::runtime_error("Engine: run not yet implemented");

//  systemState = SystemState(scenario);

  // scenario->update();
  
  // instantiate known instances

  advance();


}

void Engine::simulate(const SystemState* systemState) {
  // copy system state and set assumed time of
  throw std::runtime_error("Engine: simulate not yet implemented");
  advance();
}

void Engine::resume() {
  advance();
}

void Engine::advance() {
  while ( systemState->isAlive() ) {
    // make sure new data is added to system state
    const_cast<BPMNOS::Model::Scenario*>(systemState->scenario)->update();

    for (auto& [process,status] : systemState->getInstantiations() ) {
      systemState->instances.push_back(std::make_unique<StateMachine>(process,status));
      systemState->instances.back()->advance();
    }
/*
    // loop until TerminationEvent is received
    // listen to event
    std::unique_ptr<Event> event = listen( systemState );

    if ( event ) {
      event->processBy(this);
    }
    else {
      // wait until next time step
      // TODO
    }
*/
  }
}

/*
void Engine::start(BPMNOS::number time) {
  systemState.currentTime = time;
//  systemState.simulationTime = systemState.currentTime;

  while ( true ) {
    // loop until TerminationEvent is received

    // listen to event
    std::unique_ptr<Event> event = listen( systemState );

    if ( auto instantiationEvent = event.get()->is<InstantiationEvent>(); instantiationEvent ) {
      // create a StateMachine for the instantiated process
      auto instance = systemState.addStateMachine(instantiationEvent);
      instance->run(instance->tokens.back().get());
    }
    else if ( event.get()->is<TerminationEvent>() ) {
      // terminate execution run
      break;
    }
    else if ( event.get()->is<ClockTickEvent>() ) {
      // increase clockTime 
//      systemState.currentTime += clockTick;
      event->processBy(this);
    }
    else {
      // delegate event processing to relevant state machine

      // obtain non-const stateMachine the event is referring to
      StateMachine* stateMachine = const_cast<StateMachine*>(event.get()->token->owner);
      bool isRunning = stateMachine->run(event.get());
      if ( !isRunning ) {
        // remove state machine from system state
        erase<StateMachine>(systemState.instances,stateMachine);
      }
    }
  }

}
*/

void Engine::process(const ChoiceEvent& event) {
  throw std::runtime_error("Engine: ChoiceEvent not yet implemented");
}

void Engine::process(const ClockTickEvent& event) {
  systemState->currentTime += clockTick;
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

void Engine::process(const InstantiationEvent& event) {
  throw std::runtime_error("Engine: InstantiationEvent not yet implemented");
}
void Engine::process(const MessageDeliveryEvent& event) {
  throw std::runtime_error("Engine: MessageDeliveryEvent not yet implemented");
}
void Engine::process(const ReadyEvent& event) {
  throw std::runtime_error("Engine: ReadyEvent not yet implemented");
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
  return systemState;
}


