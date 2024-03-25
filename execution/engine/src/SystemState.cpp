#include "SystemState.h"
#include "Engine.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
  , objective(0)
{
}

SystemState::~SystemState() {
//std::cerr << "~SystemState()" << std::endl;
/*
  tokensAwaitingBoundaryEvent.clear();
  tokenAtAssociatedActivity.clear();
  tokensAwaitingStateMachineCompletion.clear();
  tokensAwaitingGatewayActivation.clear();
  tokensAwaitingJobEntryEvent.clear();
*/
}

BPMNOS::number SystemState::getTime() const {
  return assumedTime ? assumedTime.value() : currentTime;
}

bool SystemState::isAlive() const {
  if ( !scenario->isCompleted(getTime()) ) {
    return true;
  }
  return !instances.empty();
};

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > SystemState::getInstantiations() const {
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

