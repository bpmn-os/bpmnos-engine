#include "StochasticScenario.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <functional>

using namespace BPMNOS::Model;

StochasticScenario::StochasticScenario(
  const Model* model,
  BPMNOS::number earliestInstantiationTime,
  BPMNOS::number latestInstantiationTime,
  const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap,
  unsigned int seed
)
  : scenarioSeed(seed)
  , earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
  this->model = model;
  globals.resize(model->attributes.size());
  for (auto& [attribute, value] : globalValueMap) {
    globals[attribute->index] = value;
  }
}

void StochasticScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId,
                                      BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void StochasticScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute,
                                   std::optional<BPMNOS::number> value) {
  instances[(size_t)instanceId].values[attribute] = value;
}

void StochasticScenario::setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node,
                                        BPMNOS::number disclosureTime) {
  disclosure[(size_t)instanceId][node] = disclosureTime;
}

void StochasticScenario::addPendingDisclosure(const BPMNOS::number instanceId,
                                               StochasticPendingDisclosure&& pending) {
  pendingDisclosures[(size_t)instanceId].push_back(std::move(pending));
}

void StochasticScenario::addCompletionExpression(const BPMNOS::number instanceId,
                                                  const BPMN::Node* task,
                                                  CompletionExpression&& expr) {
  completionExpressions[(size_t)instanceId][task].push_back(std::move(expr));
}

std::mt19937& StochasticScenario::getRng(size_t instanceId, const BPMN::Node* node) const {
  auto key = std::make_pair(instanceId, node);
  if (!rngs.contains(key)) {
    // Create RNG seeded from scenarioSeed combined with instanceId and node hash
    size_t nodeHash = std::hash<std::string>{}(node->id);
    size_t combinedSeed = static_cast<size_t>(scenarioSeed) ^ (instanceId * 31) ^ (nodeHash * 17);
    rngs[key] = std::mt19937(combinedSeed);
  }
  return rngs[key];
}

BPMNOS::number StochasticScenario::getEarliestInstantiationTime() const {
  return earliestInstantiationTime;
}

bool StochasticScenario::isCompleted(const BPMNOS::number currentTime) const {
  return currentTime > latestInstantiationTime;
}

std::vector<const Scenario::InstanceData*> StochasticScenario::getCreatedInstances(
    const BPMNOS::number currentTime) const {
  std::vector<const Scenario::InstanceData*> result;
  for (auto& [id, instance] : instances) {
    if (instance.instantiationTime <= currentTime) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector<const Scenario::InstanceData*> StochasticScenario::getKnownInstances(
    [[maybe_unused]] const BPMNOS::number currentTime) const {
  std::vector<const Scenario::InstanceData*> result;
  for (auto& [id, instance] : instances) {
    result.push_back(&instance);
  }
  return result;
}

std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>>
StochasticScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>> result;
  for (auto& [id, instance] : instances) {
    BPMNOS::number effectiveInstantiationTime = instance.instantiationTime;
    if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(instance.process)) {
      effectiveInstantiationTime = std::max(effectiveInstantiationTime,
                                             disclosure.at(instance.id).at(instance.process));
    }
    if (effectiveInstantiationTime == currentTime) {
      auto status = getKnownInitialStatus(&instance, currentTime);
      if (effectiveInstantiationTime > instance.instantiationTime) {
        status[ExtensionElements::Index::Timestamp] = currentTime;
      }
      result.push_back({instance.process, std::move(status), getKnownInitialData(&instance, currentTime)});
    }
  }
  return result;
}

BPMNOS::Values StochasticScenario::getKnownInitialStatus(const Scenario::InstanceData* instance,
                                                          const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for (auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->attributes) {
    result.push_back(getKnownValue(instance, attribute.get(), currentTime));
  }
  return result;
}

BPMNOS::Values StochasticScenario::getKnownInitialData(const Scenario::InstanceData* instance,
                                                        const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for (auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->data) {
    result.push_back(getKnownValue(instance, attribute.get(), currentTime));
  }
  return result;
}

std::optional<BPMNOS::number> StochasticScenario::getKnownValue(
    const Scenario::InstanceData* instance,
    const BPMNOS::Model::Attribute* attribute,
    [[maybe_unused]] const BPMNOS::number currentTime) const {
  if (attribute->expression && attribute->expression->type == Expression::Type::ASSIGN) {
    std::vector<double> variableValues;
    for (auto input : attribute->expression->variables) {
      if (!input->isImmutable) {
        return std::nullopt;
      }
      auto value = getKnownValue(instance, input, currentTime);
      if (!value.has_value()) {
        return std::nullopt;
      }
      variableValues.push_back((double)value.value());
    }

    std::vector<std::vector<double>> collectionValues;
    for (auto input : attribute->expression->collections) {
      if (!input->isImmutable) {
        return std::nullopt;
      }
      collectionValues.push_back({});
      auto collection = getKnownValue(instance, input, currentTime);
      if (!collection.has_value()) {
        return std::nullopt;
      }
      for (auto value : collectionRegistry[(size_t)collection.value()]) {
        collectionValues.back().push_back(value);
      }
    }

    return number(attribute->expression->compiled.evaluate(variableValues, collectionValues));
  }
  else {
    if (instance->values.contains(attribute)) {
      return instance->values.at(attribute);
    }
  }
  return std::nullopt;
}

std::optional<BPMNOS::number> StochasticScenario::getKnownValue(
    const BPMNOS::number instanceId,
    const BPMNOS::Model::Attribute* attribute,
    const BPMNOS::number currentTime) const {
  return getKnownValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> StochasticScenario::getKnownValues(
    const BPMNOS::number instanceId,
    const BPMN::Node* node,
    const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node)) {
    if (currentTime < disclosure.at(instance.id).at(node)) {
      return std::nullopt;
    }
  }
  Values result;
  for (auto& attribute : node->extensionElements->as<const ExtensionElements>()->attributes) {
    result.push_back(getKnownValue(&instance, attribute.get(), currentTime));
  }
  return result;
}

std::optional<BPMNOS::Values> StochasticScenario::getKnownData(
    const BPMNOS::number instanceId,
    const BPMN::Node* node,
    const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node)) {
    if (currentTime < disclosure.at(instance.id).at(node)) {
      return std::nullopt;
    }
  }
  Values result;
  for (auto& attribute : node->extensionElements->as<const ExtensionElements>()->data) {
    result.push_back(getKnownValue(&instance, attribute.get(), currentTime));
  }
  return result;
}

void StochasticScenario::setTaskCompletionStatus(
    const BPMNOS::number instanceId,
    const BPMN::Node* task,
    BPMNOS::Values status) const {

  size_t id = (size_t)instanceId;

  // Check if we have completion expressions for this (instance, task)
  if (completionExpressions.contains(id) &&
      completionExpressions.at(id).contains(task) &&
      !completionExpressions.at(id).at(task).empty()) {

    auto& taskCompletionExpressions = completionExpressions.at(id).at(task);

    // Get the task's extension elements for data
    auto extensionElements = task->extensionElements->as<const ExtensionElements>();

    // Build data values from instance
    auto& instance = instances.at(id);
    Values data;
    for (auto& attribute : extensionElements->data) {
      if (instance.values.contains(attribute.get())) {
        data.push_back(instance.values.at(attribute.get()));
      }
      else {
        data.push_back(std::nullopt);
      }
    }

    // Set RNG context for random functions
    auto& randomGenerator = getRng(id, task);
    if (randomFactory) {
      randomFactory->setCurrentRng(&randomGenerator);
    }

    // Evaluate completion expressions and update status
    for (auto& completionExpression : taskCompletionExpressions) {
      auto value = completionExpression.expression->execute(status, data, globals);
      if (value.has_value() && completionExpression.attribute) {
        // Find attribute index in status
        size_t index = 0;
        for (auto& attribute : extensionElements->attributes) {
          if (attribute.get() == completionExpression.attribute) {
            status[index] = value;
            break;
          }
          ++index;
        }
      }
    }

    // Clear RNG context
    if (randomFactory) {
      randomFactory->setCurrentRng(nullptr);
    }
  }

  // Store the (possibly modified) status
  taskCompletionStatus[{id, task}] = std::move(status);
}

void StochasticScenario::revealData(BPMNOS::number currentTime) const {
  for (auto& [instanceId, pendings] : pendingDisclosures) {
    auto& instance = instances.at(instanceId);

    // Build status and data vectors for expression evaluation
    Values status;
    for (auto& attribute : instance.process->extensionElements->as<const ExtensionElements>()->attributes) {
      if (instance.values.contains(attribute.get())) {
        status.push_back(instance.values.at(attribute.get()));
      }
      else {
        status.push_back(std::nullopt);
      }
    }

    Values data;
    for (auto& attribute : instance.process->extensionElements->as<const ExtensionElements>()->data) {
      if (instance.values.contains(attribute.get())) {
        data.push_back(instance.values.at(attribute.get()));
      }
      else {
        data.push_back(std::nullopt);
      }
    }

    // Process pending disclosures that are due
    auto it = pendings.begin();
    while (it != pendings.end()) {
      if (currentTime >= it->disclosureTime) {
        // Set RNG context for random functions (use process as node for disclosure)
        auto& randomGenerator = getRng(instanceId, instance.process);
        if (randomFactory) {
          randomFactory->setCurrentRng(&randomGenerator);
        }

        // Evaluate expression
        auto value = it->expression->execute(status, data, globals);
        instance.values[it->attribute] = value;
        disclosedAttributes.insert({instanceId, it->attribute});

        // Clear RNG context
        if (randomFactory) {
          randomFactory->setCurrentRng(nullptr);
        }

        it = pendings.erase(it);
      }
      else {
        ++it;
      }
    }
  }
}
