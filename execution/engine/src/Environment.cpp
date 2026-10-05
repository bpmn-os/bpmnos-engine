#include "Environment.h"
#include "execution/engine/src/Mediator.h"
#include "execution/engine/src/SystemState.h"
#include <cassert>

using namespace BPMNOS::Execution;

void Environment::setScenario(std::unique_ptr<Scenario> scenario) {
  this->scenario = std::move(scenario);
}

void Environment::connect(Mediator* mediator) {
  mediator->addSubscriber(this, Observable::Type::Event, Observable::Type::Token, Observable::Type::SystemState);
}

void Environment::notice(const Observable* observable) {
  assert(scenario);
  if (observable->getObservableType() == Observable::Type::SystemState) {
    // a freshly installed state begins with nothing enqueued
    enqueuedEvents.clear();
  }
  scenario->dataProvider->notice(observable, *scenario, enqueuedEvents);
}

std::shared_ptr<Event> Environment::dispatchEvent(const SystemState* systemState) {
  assert(scenario);
  scenario->dataProvider->dispatchEvent(systemState, *scenario, enqueuedEvents);
  return dequeueEvent();
}

std::shared_ptr<Event> Environment::advance(const SystemState* systemState) {
  assert(scenario);
  scenario->dataProvider->advance(systemState, *scenario, enqueuedEvents);
  return dequeueEvent();
}

std::shared_ptr<Event> Environment::dequeueEvent() {
  if (enqueuedEvents.empty()) {
    return nullptr;
  }
  auto event = enqueuedEvents.front();
  enqueuedEvents.pop_front();
  return event;
}
