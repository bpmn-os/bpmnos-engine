#include "LegacyScenario.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <limits>

using namespace BPMNOS::Model;

LegacyScenario::LegacyScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const DataInput& attributes, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap)
  : attributes(attributes)
  , earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
  this->model = model;
  globals.resize(model->attributes.size());
  for ( auto& [ attribute, value ] : globalValueMap ) {
    globals[attribute->index] = value;
  }
}

LegacyScenario::LegacyScenario(const LegacyScenario& other)
  : attributes(other.attributes)
  , earliestInstantiationTime(other.earliestInstantiationTime)
  , latestInstantiationTime(other.latestInstantiationTime)
{
  this->model = other.model;
  globals = other.globals;
  for (auto& [identifier, instance] : other.instances) {
    instances[(size_t)identifier] = {instance.process, instance.id, instance.instantiationTime, instance.values};
  }
}

void LegacyScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
  auto& instanceData = instances[(size_t)instanceId];
  // initialize all attribute values to nullopt
  assert( attributes.contains(process) );
  for ( auto& [id, attribute] : attributes.at(process) ) {
    instanceData.values[attribute] = std::nullopt;
  }
}

void LegacyScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  auto& instanceData = instances[(size_t)instanceId];
  instanceData.values[attribute] = value;
}

const Model* LegacyScenario::getModel() const {
  return model;
}

BPMNOS::number LegacyScenario::getEarliestInstantiationTime() const {
  return earliestInstantiationTime;
}

bool LegacyScenario::isCompleted(const BPMNOS::number currentTime) const {
  return currentTime > latestInstantiationTime;
}

std::vector< const Scenario::InstanceData* > LegacyScenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector< const Scenario::InstanceData* > createdInstances;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime <= currentTime ) {
      createdInstances.push_back(&instance);
    }
  }
  return createdInstances;
}

std::vector< const Scenario::InstanceData* > LegacyScenario::getKnownInstances([[maybe_unused]] const BPMNOS::number currentTime) const {
  // For static scenarios, all instances are known from the start
  std::vector< const Scenario::InstanceData* > knownInstances;
  for ( auto& [id, instance] : instances ) {
    knownInstances.push_back(&instance);
  }
  return knownInstances;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > LegacyScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > instantiations;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime == currentTime ) {
      instantiations.push_back({instance.process, getKnownInitialStatus(&instance, currentTime), getKnownInitialData(&instance, currentTime)});
    }
  }
  return instantiations;
}

BPMNOS::Values LegacyScenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initialStatus;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    initialStatus.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return initialStatus;
}

BPMNOS::Values LegacyScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values initialData;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    initialData.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return initialData;
}

std::optional<BPMNOS::number> LegacyScenario::getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, [[maybe_unused]] const BPMNOS::number currentTime) const {
  if ( attribute->expression && attribute->expression->type == Expression::Type::ASSIGN ) {
    // value is obtained by an assignment
    // collect variable values
    std::vector< double > variableValues;
    for ( auto input : attribute->expression->variables ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;
      }
      auto value = getKnownValue(instance, input, currentTime);
      if ( !value.has_value() ) {
        // return nullopt because required attribute value is not given
        return std::nullopt;
      }
      variableValues.push_back( (double)value.value() );
    }

    // collect values of all variables in collection
    std::vector< std::vector< double > > collectionValues;
    for ( auto input : attribute->expression->collections ) {
      if ( !input->isImmutable ) {
        // return nullopt because required attribute value only becomes known upon execution
        return std::nullopt;
      }
      collectionValues.push_back( {} );
      auto collection = getKnownValue(instance, input, currentTime);
      if ( !collection.has_value() ) {
        // return nullopt because required collection is not given
        return std::nullopt;
      }
      for ( auto value : collectionRegistry[(size_t)collection.value()] ) {
        collectionValues.back().push_back( value );
      }
    }

    return number(attribute->expression->compiled.evaluate(variableValues,collectionValues));
  }
  else {
    // return the stored value directly
    if ( instance->values.contains(attribute) ) {
      return instance->values.at(attribute);
    }
  }

  return std::nullopt;
}

std::optional<BPMNOS::number> LegacyScenario::getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);
  return getKnownValue(&instanceData, attribute, currentTime);
}

std::optional<BPMNOS::Values> LegacyScenario::getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    values.push_back( getKnownValue(&instanceData, attribute.get(), currentTime) );
  }

  return values;
}

std::optional<BPMNOS::Values> LegacyScenario::getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instanceData = instances.at((size_t)instanceId);

  Values values;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    values.push_back( getKnownValue(&instanceData, attribute.get(), currentTime) );
  }

  return values;
}
