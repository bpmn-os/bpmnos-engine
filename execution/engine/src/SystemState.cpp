#include "SystemState.h"
#include "Engine.h"
#include "execution/utility/src/erase.h"

using namespace BPMNOS::Execution;

SystemState::SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime)
  : engine(engine)
  , scenario(scenario)
  , currentTime(currentTime)
  , contributionsToObjective(0)
  , globals(scenario->globals)
{  
}

SystemState::~SystemState() {
//std::cerr << "~SystemState()" << std::endl;
  inbox.clear();
/*
  tokensAwaitingBoundaryEvent.clear();
  tokenAtAssociatedActivity.clear();
  tokensAwaitingStateMachineCompletion.clear();
  tokensAwaitingGatewayActivation.clear();
  tokensAwaitingJobEntryEvent.clear();
*/
}

BPMNOS::number SystemState::getTime() const {
  return currentTime;
}

bool SystemState::isAlive() const {
  if ( !scenario->isCompleted(getTime()) ) {
    return true;
  }
  return !instances.empty();
};

BPMNOS::number SystemState::getObjective() const {
  auto result = contributionsToObjective;
  
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
  return scenario->getCurrentInstantiations(currentTime);
}

std::optional<BPMNOS::Values> SystemState::getStatusAttributes(const StateMachine* root, const BPMN::Node* node) const {
  return scenario->getKnownValues(root->instance.value(), node, currentTime);
}

std::optional<BPMNOS::Values> SystemState::getDataAttributes(const StateMachine* root, const BPMN::Node* node) const {
  return scenario->getKnownData(root->instance.value(), node, currentTime);
}

void SystemState::incrementTimeBy(BPMNOS::number duration) {
  currentTime += duration;
}

