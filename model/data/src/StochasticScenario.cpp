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
  : Scenario(model, globalValueMap)
  , scenarioSeed(seed)
  , earliestInstantiationTime(earliestInstantiationTime)
  , latestInstantiationTime(latestInstantiationTime)
{
}

void StochasticScenario::addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime) {
  instances[(size_t)instanceId] = {process, (size_t)instanceId, instantiationTime, {}};
}

void StochasticScenario::setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value) {
  instances[(size_t)instanceId].values[attribute] = value;
}

void StochasticScenario::setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number disclosureTime) {
  disclosure[(size_t)instanceId][node] = disclosureTime;
}

void StochasticScenario::addPendingDisclosure(const BPMNOS::number instanceId, StochasticPendingDisclosure&& pending) {
  pendingDisclosures[(size_t)instanceId].push_back(std::move(pending));
}

void StochasticScenario::addCompletionExpression(const BPMNOS::number instanceId, const BPMN::Node* task, CompletionExpression&& expr) {
  completionExpressions[(size_t)instanceId][task].push_back(std::move(expr));
}

void StochasticScenario::addArrivalExpression(const BPMNOS::number instanceId, const BPMN::Node* node, ArrivalExpression&& expr) {
  arrivalExpressions[(size_t)instanceId][node].push_back(std::move(expr));
}

void StochasticScenario::addDeferredDisclosure(const BPMNOS::number instanceId, DeferredDisclosure&& deferred) {
  deferredDisclosures[(size_t)instanceId].push_back(std::move(deferred));
}

void StochasticScenario::evaluateDeferredDisclosures() {
  if (!stochasticHandle || !randomFactory) {
    return;
  }

  for (auto& [instanceId, deferreds] : deferredDisclosures) {
    auto& instance = instances.at(instanceId);

    for (auto& deferred : deferreds) {
      // Set RNG context for this (instance, node)
      auto& rng = getRng(instanceId, deferred.node);
      randomFactory->setCurrentRng(&rng);

      // Build status/data vectors from instance values
      auto extensionElements = deferred.node->extensionElements->as<const ExtensionElements>();
      Values status, data;
      for (auto& attr : extensionElements->attributes) {
        status.push_back(instance.values.contains(attr.get()) ? instance.values.at(attr.get()) : std::nullopt);
      }
      for (auto& attr : extensionElements->data) {
        data.push_back(instance.values.contains(attr.get()) ? instance.values.at(attr.get()) : std::nullopt);
      }

      // Evaluate initialization expression
      Expression initializationExpression(*stochasticHandle, deferred.initializationExpression, extensionElements->attributeRegistry);
      BPMNOS::number value = initializationExpression.execute(status, data, globals).value_or(0);

      // Apply type conversion
      switch (deferred.attribute->type) {
        case ValueType::INTEGER:
          value = BPMNOS::number((int)value);
          break;
        case ValueType::BOOLEAN:
          value = BPMNOS::number(value != 0 ? 1 : 0);
          break;
        default:
          break;
      }

      // Evaluate disclosure expression
      Expression disclosureExpression(*stochasticHandle, deferred.disclosureExpression, extensionElements->attributeRegistry);
      BPMNOS::number ownDisclosure = disclosureExpression.execute(status, data, globals).value_or(0);

      // Compute effective disclosure (max with parent scope)
      BPMNOS::number effectiveDisclosure = ownDisclosure;
      if (auto childNode = deferred.node->represents<BPMN::ChildNode>()) {
        auto parentNode = childNode->parent;
        if (disclosure.contains(instanceId) && disclosure.at(instanceId).contains(parentNode)) {
          effectiveDisclosure = std::max(effectiveDisclosure, disclosure.at(instanceId).at(parentNode));
        }
      }

      // Update node disclosure time
      if (!disclosure[instanceId].contains(deferred.node)) {
        disclosure[instanceId][deferred.node] = effectiveDisclosure;
      } else {
        disclosure[instanceId][deferred.node] = std::max(disclosure[instanceId][deferred.node], effectiveDisclosure);
      }

      // Add as pending disclosure
      pendingDisclosures[instanceId].push_back({deferred.attribute, effectiveDisclosure, value});
    }
  }

  // Clear RNG context
  randomFactory->setCurrentRng(nullptr);

  // Clear deferred disclosures (they've been processed)
  deferredDisclosures.clear();
}

void StochasticScenario::noticeActivityArrival(BPMNOS::number instanceId, const BPMN::Node* node, const Values& status, const SharedValues& data, const Values& globals) const {
  initializeActivityData(instanceId, node, status, data, globals);
}

void StochasticScenario::initializeActivityData(BPMNOS::number instanceId, const BPMN::Node* node, const Values& status, const SharedValues& data, const Values& globals) const {
  size_t id = (size_t)instanceId;
  auto key = std::make_pair(id, node);
  auto& instance = instances.at(id);
  auto extensionElements = node->extensionElements->as<const ExtensionElements>();

  // Build full status: parent status + node's own attributes
  Values fullStatus = status;
  for (auto& attribute : extensionElements->attributes) {
    fullStatus.push_back(getValue(&instance, attribute.get(), 0));
  }

  // Apply arrival expressions if any
  if (arrivalExpressions.contains(id) && arrivalExpressions.at(id).contains(node)) {
    auto& nodeArrivalExpressions = arrivalExpressions.at(id).at(node);

    // Set RNG context for random functions
    auto& randomGenerator = getRng(id, node);
    if (randomFactory) {
      randomFactory->setCurrentRng(&randomGenerator);
    }

    // Evaluate arrival expressions and apply to full status
    for (auto& arrivalExpression : nodeArrivalExpressions) {
      auto value = arrivalExpression.expression->execute(fullStatus, data, globals);
      if (value.has_value() && arrivalExpression.attribute) {
        // Apply type conversion
        BPMNOS::number convertedValue;
        switch (arrivalExpression.attribute->type) {
          case ValueType::INTEGER:
            convertedValue = BPMNOS::number((int)value.value());
            break;
          case ValueType::BOOLEAN:
            convertedValue = BPMNOS::number(value.value() != 0 ? 1 : 0);
            break;
          default:
            convertedValue = value.value();
            break;
        }
        // Update full status at attribute's index
        fullStatus[arrivalExpression.attribute->index] = convertedValue;
      }
    }

    // Clear RNG context
    if (randomFactory) {
      randomFactory->setCurrentRng(nullptr);
    }
  }

  // Store the full computed status
  activityArrivalStatus[key] = std::move(fullStatus);
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

std::vector<const Scenario::InstanceData*> StochasticScenario::getCreatedInstances(const BPMNOS::number currentTime) const {
  std::vector<const Scenario::InstanceData*> result;
  for (auto& [id, instance] : instances) {
    if (instance.instantiationTime <= currentTime) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector<const Scenario::InstanceData*> StochasticScenario::getInstances(const BPMNOS::number currentTime) const {
  std::vector<const Scenario::InstanceData*> result;
  for (auto& [id, instance] : instances) {
    // Instance is known when process disclosure time is reached
    BPMNOS::number processDisclosure = 0;
    if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(instance.process)) {
      processDisclosure = disclosure.at(instance.id).at(instance.process);
    }
    if (currentTime >= processDisclosure) {
      result.push_back(&instance);
    }
  }
  return result;
}

std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>> StochasticScenario::getCurrentInstantiations(const BPMNOS::number currentTime) const {
  std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>> result;
  for (auto& [id, instance] : instances) {
    BPMNOS::number effectiveInstantiationTime = instance.instantiationTime;
    if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(instance.process)) {
      effectiveInstantiationTime = std::max( effectiveInstantiationTime,                                             disclosure.at(instance.id).at(instance.process) );
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

BPMNOS::Values StochasticScenario::getKnownInitialStatus(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for (auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->attributes) {
    result.push_back(getValue(instance, attribute.get(), currentTime));
  }
  return result;
}

BPMNOS::Values StochasticScenario::getKnownInitialData(const Scenario::InstanceData* instance, const BPMNOS::number currentTime) const {
  BPMNOS::Values result;
  for (auto& attribute : instance->process->extensionElements->as<const ExtensionElements>()->data) {
    result.push_back(getValue(instance, attribute.get(), currentTime));
  }
  return result;
}

std::optional<BPMNOS::number> StochasticScenario::getValue( const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, [[maybe_unused]] const BPMNOS::number currentTime) const {
  if (attribute->expression && attribute->expression->type == Expression::Type::ASSIGN) {
    std::vector<double> variableValues;
    for (auto input : attribute->expression->variables) {
      if (!input->isImmutable) {
        return std::nullopt;
      }
      auto value = getValue(instance, input, currentTime);
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
      auto collection = getValue(instance, input, currentTime);
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

std::optional<BPMNOS::number> StochasticScenario::getValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const {
  return getValue(&instances.at((size_t)instanceId), attribute, currentTime);
}

std::optional<BPMNOS::Values> StochasticScenario::getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node)) {
    if (currentTime < disclosure.at(instance.id).at(node)) {
      return std::nullopt;
    }
  }
  Values result;
  for (auto& attribute : node->extensionElements->as<const ExtensionElements>()->attributes) {
    result.push_back(getValue(&instance, attribute.get(), currentTime));
  }
  return result;
}

std::optional<BPMNOS::Values> StochasticScenario::getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const {
  auto& instance = instances.at((size_t)instanceId);
  if (disclosure.contains(instance.id) && disclosure.at(instance.id).contains(node)) {
    if (currentTime < disclosure.at(instance.id).at(node)) {
      return std::nullopt;
    }
  }
  Values result;
  for (auto& attribute : node->extensionElements->as<const ExtensionElements>()->data) {
    result.push_back(getValue(&instance, attribute.get(), currentTime));
  }
  return result;
}

void StochasticScenario::noticeRunningTask(BPMNOS::number instanceId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const {
  setTaskCompletionStatus(instanceId, task, status, data, globals);
}

void StochasticScenario::setTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const {

  size_t id = (size_t)instanceId;
  Values modifiedStatus = status;

  // Check if we have completion expressions for this (instance, task)
  if (completionExpressions.contains(id) &&
      completionExpressions.at(id).contains(task) &&
      !completionExpressions.at(id).at(task).empty()) {

    auto& taskCompletionExpressions = completionExpressions.at(id).at(task);

    // Set RNG context for random functions
    auto& randomGenerator = getRng(id, task);
    if (randomFactory) {
      randomFactory->setCurrentRng(&randomGenerator);
    }

    // Evaluate completion expressions and update status
    for (auto& completionExpression : taskCompletionExpressions) {
      auto value = completionExpression.expression->execute(modifiedStatus, data, globals);
      if (value.has_value() && completionExpression.attribute) {
        // Apply type conversion
        BPMNOS::number convertedValue;
        switch (completionExpression.attribute->type) {
          case ValueType::INTEGER:
            convertedValue = BPMNOS::number((int)value.value());
            break;
          case ValueType::BOOLEAN:
            convertedValue = BPMNOS::number(value.value() != 0 ? 1 : 0);
            break;
          default:
            convertedValue = value.value();
            break;
        }
        // Update status at attribute's index
        modifiedStatus[completionExpression.attribute->index] = convertedValue;
      }
    }

    // Clear RNG context
    if (randomFactory) {
      randomFactory->setCurrentRng(nullptr);
    }
  }

  // Store the (possibly modified) status
  taskCompletionStatus[{id, task}] = std::move(modifiedStatus);
}

void StochasticScenario::revealData(BPMNOS::number currentTime) const {
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

std::optional<BPMNOS::Values> StochasticScenario::getActivityReadyStatus(BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::number currentTime) const {
  size_t id = (size_t)instanceId;

  // Check if node data is disclosed
  if (disclosure.contains(id) && disclosure.at(id).contains(activity)) {
    if (currentTime < disclosure.at(id).at(activity)) {
      return std::nullopt;
    }
  }

  // Return stored arrival status
  auto key = std::make_pair(id, activity);
  if (!activityArrivalStatus.contains(key)) {
    return std::nullopt;
  }

  return activityArrivalStatus.at(key);
}
