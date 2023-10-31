#include "SystemState.h"
#include "Engine.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
{
}

BPMNOS::number SystemState::getTime() const {
  return assumedTime ? assumedTime.value() : currentTime;
}

bool SystemState::isAlive() const {
  if ( scenario->isCompleted(getTime()) ) {
    return false;
  }
  return !instances.empty();
};

void SystemState::addInstances() {
  for (auto& [process,status] : getInstantiations() ) {
    instantiationCounter++;
    instances.push_back(StateMachine(this,process,status));
  }
}

void SystemState::deleteInstance(StateMachine* instance) {
  // Find the iterator pointing to the instance
  auto it = std::find_if(instances.begin(), instances.end(), [instance](const StateMachine& element) { return &element == instance; });

  if (it != instances.end()) {
    // Element found, remove it
    instances.erase(it);
  }
  else {
    throw std::runtime_error("SystemState: cannot find instance to be deleted");
  }
}

std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > SystemState::getInstantiations() const {
  if ( assumedTime ) {
    return scenario->getAnticipatedInstantiations(currentTime,assumedTime.value());
  }
  else {
    return scenario->getCurrentInstantiations(currentTime);
  }
}

void SystemState::incrementTimeBy(BPMNOS::number duration) {
  if ( assumedTime ) {
    assumedTime = assumedTime.value() + duration;
  }
  else {
    currentTime += duration;
  }
}

