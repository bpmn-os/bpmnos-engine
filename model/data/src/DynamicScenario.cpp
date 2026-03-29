#include "DynamicScenario.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Model;

DynamicScenario::DynamicScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap)
  : Scenario(model, globalValueMap)
  , earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
}

void DynamicScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void DynamicScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  instances[(size_t)instanceId].values[attribute] = value;
}

void DynamicScenario::setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number disclosureTime) {
  disclosure[(size_t)instanceId][node] = disclosureTime;
}

BPMNOS::number DynamicScenario::getEarliestInstantiationTime() const {
  return earliestInstantiationTime;
}

bool DynamicScenario::isCompleted(const BPMNOS::number currentTime) const {
  return currentTime > latestInstantiationTime;
}

std::vector< const Scenario::InstanceData* > DynamicScenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime <= currentTime ) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector< const Scenario::InstanceData* > DynamicScenario::getKnownInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    // Instance is known when process disclosure time is reached
    BPMNOS::number processDisclosure = 0;
    if ( disclosure.contains(instance.id) && disclosure.at(instance.id).contains(instance.process) ) {
      processDisclosure = disclosure.at(instance.id).at(instance.process);
    }
    if ( currentTime >= processDisclosure ) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > DynamicScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > result;
  for ( auto& [id, instance] : instances ) {
    // Effective instantiation time is max(instantiationTime, processDisclosure).
    // If process data disclosure is later than instantiation time, instantiation is delayed
    // until all process data is disclosed.
    BPMNOS::number effectiveInstantiationTime = instance.instantiationTime;
    if ( disclosure.contains(instance.id) && disclosure.at(instance.id).contains(instance.process) ) {
      effectiveInstantiationTime = std::max(effectiveInstantiationTime, disclosure.at(instance.id).at(instance.process));
    }
    if ( effectiveInstantiationTime == currentTime ) {
      auto status = getKnownInitialStatus(&instance, currentTime);
      // If instantiation was delayed due to disclosure, update timestamp to reflect actual instantiation time
      if ( effectiveInstantiationTime > instance.instantiationTime ) {
        status[ExtensionElements::Index::Timestamp] = currentTime;
      }
      result.push_back({instance.process, std::move(status), getKnownInitialData(&instance, currentTime)});
    }
  }
  return result;
}

BPMNOS::Values DynamicScenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

BPMNOS::Values DynamicScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::number> DynamicScenario::getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, [[maybe_unused]] const BPMNOS::number currentTime) const {
  // Node-level disclosure is checked by caller (getCurrentInstantiations, getKnownValues, getKnownData)
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // Value is computed from an expression
    std::vector<double> variableValues;
    for ( auto input : attribute->expression->variables ) {
      if ( !input->isImmutable ) {
        return std::nullopt;
      }
      auto value = getKnownValue(instance, input, currentTime);
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
      auto collection = getKnownValue(instance, input, currentTime);
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
    // Check pending disclosures
    if ( pendingDisclosures.contains(instance->id) ) {
      for ( auto& pending : pendingDisclosures.at(instance->id) ) {
        if ( pending.attribute == attribute && currentTime >= pending.disclosureTime ) {
          return pending.value;
        }
      }
    }
  }
  return std::nullopt;
}

std::optional<BPMNOS::number> DynamicScenario::getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  return getKnownValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> DynamicScenario::getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  // Check if node data is disclosed
  if ( disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node) ) {
    if ( currentTime < disclosure.at(instance.id).at(node) ) {
      return std::nullopt;
    }
  }
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::Values> DynamicScenario::getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  // Check if node data is disclosed
  if ( disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node) ) {
    if ( currentTime < disclosure.at(instance.id).at(node) ) {
      return std::nullopt;
    }
  }
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

void DynamicScenario::addPendingDisclosure(const BPMNOS::number instanceId, PendingDisclosure&& pending) {
  pendingDisclosures[(size_t)instanceId].push_back(std::move(pending));
}

void DynamicScenario::noticeActivityArrival(
    BPMNOS::number instanceId,
    const BPMN::Node* node,
    const Values& status,
    [[maybe_unused]] const Values& data,
    [[maybe_unused]] const Values& globals) const {
  // Store parent status for getActivityReadyStatus
  activityArrivalStatus[{(size_t)instanceId, node}] = status;
}

void DynamicScenario::noticeActivityArrival(
    BPMNOS::number instanceId,
    const BPMN::Node* node,
    const Values& status,
    [[maybe_unused]] const SharedValues& data,
    [[maybe_unused]] const Values& globals) const {
  // Store parent status for getActivityReadyStatus
  activityArrivalStatus[{(size_t)instanceId, node}] = status;
}

void DynamicScenario::revealData(BPMNOS::number currentTime) const {
  for (auto& [instanceId, pendings] : pendingDisclosures) {
    auto& instance = instances.at(instanceId);

    // Process pending disclosures that are due
    auto it = pendings.begin();
    while (it != pendings.end()) {
      if (currentTime >= it->disclosureTime) {
        // Reveal pre-computed value
        instance.values[it->attribute] = it->value;
        disclosedAttributes.insert({instanceId, it->attribute});
        it = pendings.erase(it);
      }
      else {
        ++it;
      }
    }
  }
}

std::optional<BPMNOS::Values> DynamicScenario::getActivityReadyStatus(
  BPMNOS::number instanceId,
  const BPMN::Node* activity,
  BPMNOS::number currentTime
) const {
  size_t id = (size_t)instanceId;

  // Check if node data is disclosed
  if (disclosure.contains(id) && disclosure.at(id).contains(activity)) {
    if (currentTime < disclosure.at(id).at(activity)) {
      return std::nullopt;
    }
  }

  // Get stored arrival status (parent's status)
  auto key = std::make_pair(id, activity);
  if (!activityArrivalStatus.contains(key)) {
    return std::nullopt;
  }

  auto& instance = instances.at(id);
  auto extensionElements = activity->extensionElements->as<const BPMNOS::Model::ExtensionElements>();

  // Start with parent's status
  Values result = activityArrivalStatus.at(key);

  // Add node's own attributes (same approach as getKnownValues)
  for (auto& attribute : extensionElements->attributes) {
    result.push_back(getKnownValue(&instance, attribute.get(), currentTime));
  }

  return result;
}
