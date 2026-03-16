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

std::vector< const Scenario::InstanceData* > StaticScenario::getKnownInstances([[maybe_unused]] const BPMNOS::number currentTime) const {
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
    result.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

BPMNOS::Values StaticScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for ( auto& attribute : instance->process->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getKnownValue(instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::number> StaticScenario::getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, [[maybe_unused]] const BPMNOS::number currentTime) const {
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
  }
  return std::nullopt;
}

std::optional<BPMNOS::number> StaticScenario::getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  return getKnownValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> StaticScenario::getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::Values> StaticScenario::getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}
