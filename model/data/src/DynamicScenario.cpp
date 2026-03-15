#include "DynamicScenario.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Model;

DynamicScenario::DynamicScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap)
  : earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
  this->model = model;
  globals.resize(model->attributes.size());
  for ( auto& [attribute, value] : globalValueMap ) {
    globals[attribute->index] = value;
  }
}

void DynamicScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void DynamicScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  instances[(size_t)instanceId].values[attribute] = value;
}

void DynamicScenario::setDisclosure(const BPMNOS::number instanceId, const Attribute* attribute, BPMNOS::number disclosureTime) {
  disclosureTimes[(size_t)instanceId][attribute] = disclosureTime;
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

std::vector< const Scenario::InstanceData* > DynamicScenario::getKnownInstances([[maybe_unused]] const BPMNOS::number currentTime) const {
  // All instances are known from the start
  std::vector< const Scenario::InstanceData* > result;
  for ( auto& [id, instance] : instances ) {
    result.push_back(&instance);
  }
  return result;
}

std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > DynamicScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > result;
  for ( auto& [id, instance] : instances ) {
    if ( instance.instantiationTime == currentTime ) {
      // Check if all process attributes are disclosed
      bool allDisclosed = true;
      if ( disclosureTimes.contains(instance.id) ) {
        for ( auto& [attribute, disclosureTime] : disclosureTimes.at(instance.id) ) {
          if ( currentTime < disclosureTime ) {
            allDisclosed = false;
            break;
          }
        }
      }
      if ( allDisclosed ) {
        result.push_back({instance.process, getKnownInitialStatus(&instance, currentTime), getKnownInitialData(&instance, currentTime)});
      }
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

std::optional<BPMNOS::number> DynamicScenario::getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  // Check if attribute value has been disclosed yet
  if ( disclosureTimes.contains(instance->id) && disclosureTimes.at(instance->id).contains(attribute) ) {
    if ( currentTime < disclosureTimes.at(instance->id).at(attribute) ) {
      return std::nullopt; // Not yet disclosed
    }
  }

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

std::optional<BPMNOS::number> DynamicScenario::getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  return getKnownValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> DynamicScenario::getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    if ( disclosureTimes.contains(instance.id) && disclosureTimes.at(instance.id).contains(attribute.get()) ) {
      if ( currentTime < disclosureTimes.at(instance.id).at(attribute.get()) ) {
        return std::nullopt;
      }
    }
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

std::optional<BPMNOS::Values> DynamicScenario::getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    if ( disclosureTimes.contains(instance.id) && disclosureTimes.at(instance.id).contains(attribute.get()) ) {
      if ( currentTime < disclosureTimes.at(instance.id).at(attribute.get()) ) {
        return std::nullopt;
      }
    }
    result.push_back( getKnownValue(&instance, attribute.get(), currentTime) );
  }
  return result;
}

void DynamicScenario::addPendingDisclosure(const BPMNOS::number instanceId, PendingDisclosure&& pending) {
  pendingDisclosures[(size_t)instanceId].push_back(std::move(pending));
}

void DynamicScenario::revealData(BPMNOS::number currentTime) const {
  for ( auto& [instanceId, pendings] : pendingDisclosures ) {
    auto& instance = instances.at(instanceId);

    // Build status and data vectors for expression evaluation
    Values status;
    for ( auto& attribute : instance.process->extensionElements->as<const ExtensionElements>()->attributes ) {
      if ( instance.values.contains(attribute.get()) ) {
        status.push_back(instance.values.at(attribute.get()));
      }
      else {
        status.push_back(std::nullopt);
      }
    }

    Values data;
    for ( auto& attribute : instance.process->extensionElements->as<const ExtensionElements>()->data ) {
      if ( instance.values.contains(attribute.get()) ) {
        data.push_back(instance.values.at(attribute.get()));
      }
      else {
        data.push_back(std::nullopt);
      }
    }

    // Process pending disclosures that are due
    auto it = pendings.begin();
    while ( it != pendings.end() ) {
      if ( currentTime >= it->disclosureTime ) {
        // Evaluate expression using existing Expression::execute
        auto value = it->expression->execute(status, data, globals);
        instance.values[it->attribute] = value;
        disclosedAttributes.insert({instanceId, it->attribute});

        // Remove from pending
        it = pendings.erase(it);
      }
      else {
        ++it;
      }
    }
  }
}
