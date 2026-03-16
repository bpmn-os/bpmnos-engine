#include "StochasticDataProvider.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/DecisionTask.h"
#include <algorithm>
#include <ranges>

using namespace BPMNOS::Model;

StochasticDataProvider::StochasticDataProvider(const std::string& modelFile,
                                               const std::string& instanceFileOrString,
                                               unsigned int seed)
  : StochasticDataProvider(modelFile, {}, instanceFileOrString, seed)
{
}

StochasticDataProvider::StochasticDataProvider(const std::string& modelFile,
                                               const std::vector<std::string>& folders,
                                               const std::string& instanceFileOrString,
                                               unsigned int seed)
  : DataProvider(modelFile, folders)
  , reader(CSVReader(instanceFileOrString))
  , seed(seed)
  , columnCount(0)
{
  initializeStochasticHandle();
  readInstances();
}

void StochasticDataProvider::initializeStochasticHandle() {
  // Copy lookup tables from model
  for (auto& lookupTable : model->lookupTables) {
    auto* table = lookupTable.get();
    stochasticHandle.add(table->name, [table](const std::vector<double>& args) {
      return table->at(args);
    });
  }

  // Register random functions (skips if name already taken by lookup table)
  randomFactory.registerFunctions(stochasticHandle);
}

void StochasticDataProvider::readInstances() {
  CSVReader::Table table = reader.read();
  if (table.empty()) {
    throw std::runtime_error("StochasticDataProvider: table '" + reader.instanceFileOrString + "' is empty");
  }

  if (table.size() < 2) {
    throw std::runtime_error("StochasticDataProvider: table must have at least a header and one data row");
  }

  columnCount = table[0].size();
  if (columnCount != 3 && columnCount != 4 && columnCount != 6) {
    throw std::runtime_error("StochasticDataProvider: expected 3, 4, or 6 columns, got " + std::to_string(columnCount));
  }

  // Column indices
  enum { INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, ARRIVAL, COMPLETION };

  for (auto& row : table | std::views::drop(1)) {
    if (row.empty()) {
      continue;
    }
    if (row.size() != columnCount) {
      throw std::runtime_error("StochasticDataProvider: inconsistent number of cells");
    }

    // Get instance ID
    if (!std::holds_alternative<std::string>(row.at(INSTANCE_ID))) {
      throw std::runtime_error("StochasticDataProvider: illegal instance id");
    }
    std::string instanceIdentifier = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID
    if (!std::holds_alternative<std::string>(row.at(NODE_ID))) {
      throw std::runtime_error("StochasticDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization expression
    if (!std::holds_alternative<std::string>(row.at(INITIALIZATION))) {
      throw std::runtime_error("StochasticDataProvider: illegal initialization");
    }
    std::string initializationString = std::get<std::string>(row.at(INITIALIZATION));

    // Get disclosure (if 4+ columns)
    std::string disclosureExpression;
    if (columnCount >= 4) {
      if (!std::holds_alternative<std::string>(row.at(DISCLOSURE))) {
        throw std::runtime_error("StochasticDataProvider: illegal disclosure");
      }
      disclosureExpression = std::get<std::string>(row.at(DISCLOSURE));
    }

    // Get arrival (if 6 columns)
    std::string arrivalExpression;
    if (columnCount == 6) {
      if (!std::holds_alternative<std::string>(row.at(ARRIVAL))) {
        throw std::runtime_error("StochasticDataProvider: illegal arrival");
      }
      arrivalExpression = std::get<std::string>(row.at(ARRIVAL));
    }

    // Get completion (if 6 columns)
    std::string completionExpression;
    if (columnCount == 6) {
      if (!std::holds_alternative<std::string>(row.at(COMPLETION))) {
        throw std::runtime_error("StochasticDataProvider: illegal completion");
      }
      completionExpression = std::get<std::string>(row.at(COMPLETION));
    }

    // Handle global attributes
    if (instanceIdentifier.empty() && nodeId.empty()) {
      if (!disclosureExpression.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have disclosure");
      }
      if (!arrivalExpression.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have arrival");
      }
      if (!completionExpression.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have completion");
      }
      if (initializationString.empty()) {
        continue;
      }
      evaluateGlobal(initializationString);
    }
    else if (instanceIdentifier.empty()) {
      throw std::runtime_error("StochasticDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdentifier, STRING);
      BPMN::Node* node = findNode(nodeId);

      // First occurrence of instance must have node = process
      if (!instances.contains(instanceId)) {
        if (!node->represents<BPMN::Process>()) {
          throw std::runtime_error("StochasticDataProvider: first row for instance '" + instanceIdentifier +
                                   "' must reference a process node, got '" + nodeId + "'");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StochasticInstanceData{process, instanceId,
                                                       std::numeric_limits<BPMNOS::number>::max(), {}};
        disclosure[instanceId][process] = 0;
      }

      auto& instance = instances[instanceId];

      // Handle COMPLETION expression (only valid for Tasks, not SendTask/ReceiveTask/DecisionTask)
      if (!completionExpression.empty()) {
        if (!node->represents<BPMN::Task>() ||
            node->represents<BPMN::SendTask>() ||
            node->represents<BPMN::ReceiveTask>() ||
            node->represents<DecisionTask>()) {
          throw std::runtime_error("StochasticDataProvider: COMPLETION only valid for Task nodes, not '" +
                                   nodeId + "'");
        }

        auto [attributeName, expressionString] = DataProvider::parseInitialization(completionExpression);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attributeName)) {
          throw std::runtime_error("StochasticDataProvider: node '" + nodeId +
                                   "' has no attribute '" + attributeName + "'");
        }

        auto attribute = extensionElements->attributeRegistry[attributeName];
        auto expression = std::make_unique<Expression>(stochasticHandle, expressionString,
                                                       extensionElements->attributeRegistry);
        completionExpressions[instanceId][node].push_back({attribute, std::move(expression)});
      }

      // Handle ARRIVAL expression (valid for all Activity types)
      if (!arrivalExpression.empty()) {
        if (!node->represents<BPMN::Activity>()) {
          throw std::runtime_error("StochasticDataProvider: ARRIVAL only valid for Activity nodes, not '" +
                                   nodeId + "'");
        }

        auto [attributeName, expressionString] = DataProvider::parseInitialization(arrivalExpression);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attributeName)) {
          throw std::runtime_error("StochasticDataProvider: node '" + nodeId +
                                   "' has no attribute '" + attributeName + "'");
        }

        auto attribute = extensionElements->attributeRegistry[attributeName];
        if (attribute->expression) {
          throw std::runtime_error("StochasticDataProvider: attribute '" + attributeName +
                                   "' has model expression and cannot use ARRIVAL");
        }

        // Check mutual exclusivity with INITIALIZATION
        if (instance.data.contains(attribute)) {
          throw std::runtime_error("StochasticDataProvider: attribute '" + attributeName +
                                   "' cannot use both ARRIVAL and INITIALIZATION");
        }
        if (pendingDisclosures.contains(instanceId)) {
          for (auto& pending : pendingDisclosures.at(instanceId)) {
            if (pending.attribute == attribute) {
              throw std::runtime_error("StochasticDataProvider: attribute '" + attributeName +
                                       "' cannot use both ARRIVAL and INITIALIZATION");
            }
          }
        }

        auto expression = std::make_unique<Expression>(stochasticHandle, expressionString,
                                                       extensionElements->attributeRegistry);
        arrivalExpressions[instanceId][node].push_back({attribute, std::move(expression)});
      }

      // Handle INITIALIZATION
      if (initializationString.empty()) {
        if (!disclosureExpression.empty()) {
          throw std::runtime_error("StochasticDataProvider: DISCLOSURE requires INITIALIZATION in the same row");
        }
        continue;
      }

      auto [attribute, expressionString] = lookupAttribute(node, initializationString);

      // Check mutual exclusivity with ARRIVAL
      if (arrivalExpressions.contains(instanceId) && arrivalExpressions.at(instanceId).contains(node)) {
        for (auto& arrival : arrivalExpressions.at(instanceId).at(node)) {
          if (arrival.attribute == attribute) {
            throw std::runtime_error("StochasticDataProvider: attribute '" + attribute->name +
                                     "' cannot use both INITIALIZATION and ARRIVAL");
          }
        }
      }

      // Evaluate INITIALIZATION expression first (so DISCLOSURE can reference it)
      BPMNOS::number value = evaluateExpression(instanceId, node, expressionString, attribute->type);
      parseTimeEvaluatedValues[instanceId][attribute] = value;

      // Parse disclosure time (can reference the just-initialized attribute)
      BPMNOS::number ownDisclosure = 0;
      if (!disclosureExpression.empty()) {
        ownDisclosure = evaluateExpression(instanceId, node, disclosureExpression, DECIMAL);
      }

      BPMNOS::number disclosureTime = getEffectiveDisclosure(instanceId, node, ownDisclosure);

      if (disclosureTime == 0) {
        // Immediate disclosure: store value directly
        instance.data[attribute] = value;
      }
      else {
        // Deferred disclosure: store pre-computed value for later reveal
        pendingDisclosures[instanceId].push_back({attribute, disclosureTime, value});
      }
    }
  }

  // Finalize instances
  for (auto& [id, instance] : instances) {
    ensureDefaultValue(instance, Keyword::Instance, id);
    ensureDefaultValue(instance, Keyword::Timestamp);
    instance.instantiation = instance.data.at(attributes[instance.process][Keyword::Timestamp]);

    BPMNOS::number effectiveInstantiation = instance.instantiation;
    if (disclosure.contains(id) && disclosure.at(id).contains(instance.process)) {
      effectiveInstantiation = std::max(effectiveInstantiation, disclosure.at(id).at(instance.process));
    }

    if (earliestInstantiation > effectiveInstantiation) {
      earliestInstantiation = effectiveInstantiation;
    }
    if (latestInstantiation < effectiveInstantiation) {
      latestInstantiation = effectiveInstantiation;
    }
  }
}

BPMNOS::number StochasticDataProvider::evaluateExpression(const std::string& expressionString) const {
  return DataProvider::evaluateExpression(expressionString, stochasticHandle);
}

void StochasticDataProvider::evaluateGlobal(const std::string& initializationString) {
  DataProvider::evaluateGlobal(initializationString, stochasticHandle);
}

BPMNOS::number StochasticDataProvider::evaluateExpression(
    size_t instanceId,
    const BPMN::Node* node,
    const std::string& expressionString,
    ValueType type) const {
  return DataProvider::evaluateExpression(instanceId, node, expressionString, type, stochasticHandle);
}

BPMNOS::number StochasticDataProvider::getEffectiveDisclosure(size_t instanceId, const BPMN::Node* node,
                                                               BPMNOS::number ownDisclosure) {
  BPMNOS::number effectiveDisclosure = ownDisclosure;

  if (auto childNode = node->represents<BPMN::ChildNode>()) {
    auto parentNode = childNode->parent;
    if (!disclosure[instanceId].contains(parentNode)) {
      throw std::runtime_error("StochasticDataProvider: disclosure for '" + node->id +
                               "' given before parent '" + parentNode->id + "'");
    }
    effectiveDisclosure = std::max(effectiveDisclosure, disclosure[instanceId][parentNode]);
  }

  if (!disclosure[instanceId].contains(node)) {
    disclosure[instanceId][node] = effectiveDisclosure;
  }
  else {
    disclosure[instanceId][node] = std::max(disclosure[instanceId][node], effectiveDisclosure);
  }

  return effectiveDisclosure;
}

std::unique_ptr<Scenario> StochasticDataProvider::createScenario(unsigned int scenarioId) {
  auto scenario = std::make_unique<StochasticScenario>(model.get(), earliestInstantiation,
                                                        latestInstantiation, globalValueMap,
                                                        seed + scenarioId);

  // Set the random factory reference
  scenario->randomFactory = &randomFactory;

  for (auto& [id, instance] : instances) {
    auto& timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    auto instantiationTime = instance.data[timestampAttribute];
    scenario->addInstance(instance.process, id, instantiationTime);
    for (auto& [attribute, value] : instance.data) {
      scenario->setValue(id, attribute, value);
    }
  }

  // Set node disclosure times
  for (auto& [instanceId, nodes] : disclosure) {
    for (auto& [node, disclosureTime] : nodes) {
      scenario->setDisclosure(instanceId, node, disclosureTime);
    }
  }

  // Add pending disclosures (pre-computed values)
  for (auto& [instanceId, pendings] : pendingDisclosures) {
    for (auto& pending : pendings) {
      scenario->addPendingDisclosure(instanceId, {pending.attribute, pending.disclosureTime, pending.value});
    }
  }

  // Add completion expressions
  for (auto& [instanceId, tasks] : completionExpressions) {
    for (auto& [task, expressions] : tasks) {
      for (auto& sourceExpression : expressions) {
        auto expression = std::make_unique<Expression>(stochasticHandle,
                                                       sourceExpression.expression->expression,
                                                       sourceExpression.expression->attributeRegistry);
        scenario->addCompletionExpression(instanceId, task, {sourceExpression.attribute, std::move(expression)});
      }
    }
  }

  // Add arrival expressions
  for (auto& [instanceId, nodes] : arrivalExpressions) {
    for (auto& [node, expressions] : nodes) {
      for (auto& sourceExpression : expressions) {
        auto expression = std::make_unique<Expression>(stochasticHandle,
                                                       sourceExpression.expression->expression,
                                                       sourceExpression.expression->attributeRegistry);
        scenario->addArrivalExpression(instanceId, node, {sourceExpression.attribute, std::move(expression)});
      }
    }
  }

  // Reveal data disclosed at time 0
  scenario->revealData(0);

  return scenario;
}
