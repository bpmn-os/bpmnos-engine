#include "SystemState.h"

using namespace BPMNOS::Execution;

BPMNOS::number SystemState::getTime() const {
  return assumedTime ? assumedTime.value() : currentTime;
}

std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > SystemState::getInstantiations() const {
  if ( assumedTime ) {
    return scenario->getAssumedInstantiations(currentTime, assumedTime.value());
  }
  else {
    return scenario->getKnownInstantiations(currentTime);
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

