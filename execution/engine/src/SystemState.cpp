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

