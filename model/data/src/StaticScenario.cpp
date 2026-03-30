#include "StaticScenario.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Model;

StaticScenario::StaticScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap)
  : Scenario(model, globalValueMap)
  , earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
}

void StaticScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void StaticScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  instances[(size_t)instanceId].values[attribute] = value;
}

BPMNOS::number StaticScenario::getEarliestInstantiationTime() const {
  return earliestInstantiationTime;
}

bool StaticScenario::isCompleted(const BPMNOS::number currentTime) const {
  return currentTime > latestInstantiationTime;
}

std::vector< const Scenario::InstanceData* > StaticScenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime <= currentTime ) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector< const Scenario::InstanceData* > StaticScenario::getInstances([[maybe_unused]] const BPMNOS::number currentTime) const {
  // All instances are known from the start
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    result.push_back(&instance);
  }
  return result;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > StaticScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime == currentTime ) {
      result.push_back({instance.process, getKnownInitialStatus(&instance, currentTime), getKnownInitialData(&instance, currentTime)});
    }
  }
  return result;
}

BPMNOS::Values StaticScenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back( getValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

BPMNOS::Values StaticScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::number> StaticScenario::getValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, [[maybe_unused]] const BPMNOS::number currentTime) const {
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // Value is computed from an expression
    std::vector<double> variableValues;
    for ( auto input : attribute->expression->variables ) {
      if ( !input->isImmutable ) {
        return std::nullopt;
      }
      auto value = getValue(instance, input, currentTime);
      if ( !value.has_value() ) {
        return std::nullopt;
      }
      variableValues.push_back( (double)value.value() );
    }

    std::vector<std::vector<double>> collectionValues;
    for ( auto input : attribute->expression->collections ) {
      if ( !input->isImmutable ) {
        return std::nullopt;
      }
      collectionValues.push_back( {} );
      auto collection = getValue(instance, input, currentTime);
      if ( !collection.has_value() ) {
        return std::nullopt;
      }
      for ( auto value : collectionRegistry[(size_t)collection.value()] ) {
        collectionValues.back().push_back( value );
      }
    }

    return number(attribute->expression->compiled.evaluate(variableValues, collectionValues));
  }
  else {
    // Return stored value directly
    if ( instance->values.contains(attribute) ) {
      return instance->values.at(attribute);
    }
  }
  return std::nullopt;
}

std::optional<BPMNOS::number> StaticScenario::getValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  return getValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> StaticScenario::getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back( getValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::Values> StaticScenario::getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

void StaticScenario::noticeActivityArrival(
    BPMNOS::number instanceId,
    const BPMN::Node* node,
    const Values& status,
    [[maybe_unused]] const SharedValues& data,
    [[maybe_unused]] const Values& globals) const {
  // Store parent status for getActivityReadyStatus
  activityArrivalStatus[{(size_t)instanceId, node}] = status;
}

std::optional<BPMNOS::Values> StaticScenario::getActivityReadyStatus(
  BPMNOS::number instanceId,
  const BPMN::Node* activity,
  [[maybe_unused]] BPMNOS::number currentTime
) const {
  // For static scenarios, all data is known from start - no disclosure checking needed.
  // Get stored arrival status (parent's status) and extend with node's own attributes.
  auto key = std::make_pair((size_t)instanceId, activity);
  if (!activityArrivalStatus.contains(key)) {
    return std::nullopt;
  }

  auto& instance = instances.at((size_t)instanceId);
  auto extensionElements = activity->extensionElements->as<const BPMNOS::Model::ExtensionElements>();

  // Start with parent's status
  Values result = activityArrivalStatus.at(key);

  // Add node's own attributes (same approach as getStatus)
  for (auto& attribute : extensionElements->attributes) {
    result.push_back(getValue(&instance, attribute.get(), currentTime));
  }

  return result;
}
