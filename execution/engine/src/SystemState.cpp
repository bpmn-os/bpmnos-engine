#include "SystemState.h"
#include "Engine.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
  , objective(0)
  , globals(scenario->globals)
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

BPMNOS::number SystemState::getObjective() const {
  auto result = objective;
  
  for ( auto& attribute : scenario->getModel()->attributes ) {
    assert( attribute->category == BPMNOS::Model::Attribute::Category::GLOBAL );
    auto value = globals[attribute->index];
    if ( value.has_value() ) {
      result += attribute->weight * value.value();
    }
  }
  
  return result;
}

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
    return scenario->getAnticipatedValues(root->instance.value(), node, currentTime);
  }
  else {
    auto knownValues = scenario->getKnownValues(root->instance.value(), node, currentTime);
    if ( knownValues ) {
      return std::move( knownValues.value() );
    }
  }
  return std::nullopt;
}

std::optional<BPMNOS::Values> SystemState::getDataAttributes(const StateMachine* root, const BPMN::Node* node) const {
  std::optional<BPMNOS::Values> values;
  if ( assumedTime ) {
    return scenario->getAnticipatedData(root->instance.value(), node, currentTime);
  }
  else {
    auto knownData = scenario->getKnownData(root->instance.value(), node, currentTime);
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

