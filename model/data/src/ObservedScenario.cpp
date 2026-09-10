#include "ObservedScenario.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <limits>
#include <stdexcept>

using namespace BPMNOS::Model;

ObservedScenario::ObservedScenario(const Model* model, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap)
  : Scenario(model, globalValueMap)
{
}

void ObservedScenario::observeInstantiation(const BPMN::Process* process, BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void ObservedScenario::observeValue(BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  instances.at((size_t)instanceId).values[attribute] = value;
}

void ObservedScenario::observeReadyStatus(BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::Values status) {
  readyStatuses[{(size_t)instanceId, activity}] = std::move(status);
}

void ObservedScenario::observeCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, BPMNOS::Values status) {
  completionStatuses[{(size_t)instanceId, task}] = std::move(status);
}

BPMNOS::number ObservedScenario::getEarliestInstantiationTime() const {
  // No instantiation is known in advance, so no start time is too late
  return std::numeric_limits<BPMNOS::number>::max();
}

bool ObservedScenario::isCompleted([[maybe_unused]] const BPMNOS::number currentTime) const {
  return false;
}

std::vector< const Scenario::InstanceData* > ObservedScenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime <= currentTime ) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector< const Scenario::InstanceData* > ObservedScenario::getInstances(const BPMNOS::number currentTime) const {
  // Nothing is known beyond what has been observed, so the known instances are the created ones
  return getCreatedInstances(currentTime);
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > ObservedScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime == currentTime ) {
      result.push_back({instance.process, getKnownInitialStatus(&instance, currentTime), getKnownInitialData(&instance, currentTime)});
    }
  }
  return result;
}

BPMNOS::Values ObservedScenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->attributes ) {
    result.push_back( getValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

BPMNOS::Values ObservedScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->data ) {
    result.push_back( getValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::number> ObservedScenario::getValue(const Scenario::InstanceData* instance, const Attribute* attribute, const BPMNOS::number currentTime) const {
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // Value is computed from an expression declared in the model
    return getAssignedValue(instance, attribute, currentTime);
  }
  // Return the observed value, which is std::nullopt both when observed to be undefined and when not
  // yet observed at all
  if ( instance->values.contains(attribute) ) {
    return instance->values.at(attribute);
  }
  return std::nullopt;
}

std::optional<BPMNOS::number> ObservedScenario::getValue(const BPMNOS::number instanceId, const Attribute* attribute, const BPMNOS::number currentTime) const {
  return getValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> ObservedScenario::getObservedValues(const Scenario::InstanceData& instance, const std::vector< std::unique_ptr<Attribute> >& attributes, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : attributes ) {
    if ( !attribute->expression && !instance.values.contains(attribute.get()) ) {
      // Not observed yet; the engine waits and asks again at the next clock tick
      return std::nullopt;
    }
    result.push_back( getValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::Values> ObservedScenario::getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  return getObservedValues(instance, node->extensionElements->as<const ExtensionElements>()->attributes, currentTime);
}

std::optional<BPMNOS::Values> ObservedScenario::getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  return getObservedValues(instance, node->extensionElements->as<const ExtensionElements>()->data, currentTime);
}

std::optional<BPMNOS::Values> ObservedScenario::getReportedStatus(
  const std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values>& reported,
  BPMNOS::number instanceId,
  const BPMN::Node* node,
  BPMNOS::number currentTime
) const {
  auto key = std::make_pair((size_t)instanceId, node);
  if ( !reported.contains(key) ) {
    return std::nullopt;
  }
  auto& status = reported.at(key);
  if ( currentTime >= status[ExtensionElements::Index::Timestamp] ) {
    return status;
  }
  return std::nullopt;
}

std::optional<BPMNOS::Values> ObservedScenario::getActivityReadyStatus(
  [[maybe_unused]] BPMNOS::number rootId,
  BPMNOS::number instanceId,
  const BPMN::Node* activity,
  BPMNOS::number currentTime
) const {
  // keyed by the full instance id, so concurrent executions of one activity don't collide
  return getReportedStatus(readyStatuses, instanceId, activity, currentTime);
}

std::optional<BPMNOS::Values> ObservedScenario::getTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, BPMNOS::number currentTime) const {
  return getReportedStatus(completionStatuses, instanceId, task, currentTime);
}

std::unique_ptr<Scenario> ObservedScenario::clone([[maybe_unused]] BPMNOS::number spawnTime, [[maybe_unused]] size_t index) const {
  throw std::logic_error("ObservedScenario: an observed world cannot be copied");
}
