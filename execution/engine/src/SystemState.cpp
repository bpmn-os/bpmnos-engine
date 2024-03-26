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

std::optional<BPMNOS::Values> SystemState::getStatusAttributes(const StateMachine* root, const BPMN::Node* node) const {
  std::optional<BPMNOS::Values> values;
  if ( assumedTime ) {
    return scenario->getAnticipatedValues(root->instanceId, node, currentTime);
  }
  else {
    auto knownValues = scenario->getKnownValues(root->instanceId, node, currentTime);
    if ( knownValues ) {
      return std::move( knownValues.value() );
    }
  }
  return std::nullopt;
}

std::optional<BPMNOS::Values> SystemState::getDataAttributes(const StateMachine* root, const BPMN::Node* node) const {
  std::optional<BPMNOS::Values> values;
  if ( assumedTime ) {
    return scenario->getAnticipatedData(root->instanceId, node, currentTime);
  }
  else {
    auto knownData = scenario->getKnownData(root->instanceId, node, currentTime);
    if ( knownData ) {
      return std::move( knownData.value() );
    }
  }
  return std::nullopt;
}

void SystemState::incrementTimeBy(BPMNOS::number duration) {
  if ( assumedTime ) {
    assumedTime = assumedTime.value() + duration;
  }
  else {
    currentTime += duration;
  }
}

