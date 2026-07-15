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
  , reader(CSVReader(instanceFileOrString, ";"))
  , seed(seed)
  , columnCount(0)
{
  initializeStochasticHandle();
  // Set up RNG for parse-time evaluation of random expressions
  std::mt19937 parseTimeRng(seed);
  randomFactory.setCurrentRng(&parseTimeRng);
  readInstances();
}

StochasticDataProvider::StochasticDataProvider(Input input, unsigned int seed)
  : DataProvider(std::move(input.model), std::move(input.lookupTables))
  , reader(CSVReader(input.instance, ";"))
  , seed(seed)
  , columnCount(0)
{
  initializeStochasticHandle();
  // Set up RNG for parse-time evaluation of random expressions
  std::mt19937 parseTimeRng(seed);
  randomFactory.setCurrentRng(&parseTimeRng);
  readInstances();
}

void StochasticDataProvider::initializeStochasticHandle() {
  // Copy lookup tables from model
  for (auto& lookupTable : model->lookupTables) {
    auto* table = lookupTable.get();
    stochasticHandle.addFunction(table->name, [table](const std::vector<double>& args) {
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
  enum { INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, READY, COMPLETION };

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
      if (std::holds_alternative<std::string>(row.at(DISCLOSURE))) {
        disclosureExpression = std::get<std::string>(row.at(DISCLOSURE));
      }
      else if (std::holds_alternative<BPMNOS::number>(row.at(DISCLOSURE))) {
        disclosureExpression = std::to_string((double)std::get<BPMNOS::number>(row.at(DISCLOSURE)));
      }
    }

    // Get ready (if 6 columns)
    std::string readyExpression;
    if (columnCount == 6) {
      if (!std::holds_alternative<std::string>(row.at(READY))) {
        throw std::runtime_error("StochasticDataProvider: illegal ready");
      }
      readyExpression = std::get<std::string>(row.at(READY));
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
      if (!readyExpression.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have ready");
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
          throw std::runtime_error("StochasticDataProvider: '" + nodeId + "' is not a process (first row for instance '" + instanceIdentifier + "' must reference a process)");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StochasticInstanceData{process, instanceId,
                                                       std::numeric_limits<BPMNOS::number>::max(), {}};
        disclosureTimes[instanceId][process] = 0;
      }

      // Handle COMPLETION expression (valid for all Task types)
      if (!completionExpression.empty()) {
        if (!node->represents<BPMN::Task>()) {
          throw std::runtime_error("StochasticDataProvider: completion expressions are not allowed for node '" + nodeId + "'");
        }

        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        auto expression = std::make_shared<Expression>(stochasticHandle, completionExpression,
                                                       extensionElements->attributeRegistry);

        // Completion expressions must only modify STATUS attributes
        if (expression->target.has_value() &&
            expression->target.value()->category != BPMNOS::Model::Attribute::Category::STATUS) {
          throw std::runtime_error("StochasticDataProvider: completion expression for '" + nodeId + "' attempts to modify non-status attribute '" +  expression->target.value()->name + "'");
        }

        // For SendTask, ReceiveTask, DecisionTask: timestamp must not be modified (completion time is event-driven)
        if (expression->target.has_value() &&
            expression->target.value()->name == Keyword::Timestamp &&
            (node->represents<BPMN::SendTask>() ||
             node->represents<BPMN::ReceiveTask>() ||
             node->represents<DecisionTask>())) {
          throw std::runtime_error("StochasticDataProvider: completion expression must not modify timestamp for '" + nodeId + "'");
        }

        completionExpressions[instanceId][node].push_back(std::move(expression));
      }

      // Handle READY expression (valid for all Activity types)
      if (!readyExpression.empty()) {
        if (!node->represents<BPMN::Activity>()) {
          throw std::runtime_error("StochasticDataProvider: '" + nodeId + "' is not an activity (ready expressions are only allowed for activities)");
        }

        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        auto expression = std::make_shared<Expression>(stochasticHandle, readyExpression, extensionElements->attributeRegistry);

        // Ready expressions must only modify STATUS attributes
        if (expression->target.has_value() &&
            expression->target.value()->category != BPMNOS::Model::Attribute::Category::STATUS) {
          throw std::runtime_error("StochasticDataProvider: ready expression for '" + nodeId + "' attempts to modify non-status attribute '" +  expression->target.value()->name + "'");
        }


        readyExpressions[instanceId][node].push_back(std::move(expression));
      }

      // Handle INITIALIZATION
      if (initializationString.empty()) {
        if (!disclosureExpression.empty()) {
          throw std::runtime_error("StochasticDataProvider: disclosure expression requires initialization expression in the same row");
        }
      }
      else {
        auto [attribute, expressionString] = lookupAttribute(node, initializationString);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

        // Create shared expressions for per-scenario evaluation
        auto initializationExpression = std::make_shared<Expression>(stochasticHandle, expressionString, extensionElements->attributeRegistry);

        // If no DISCLOSURE expression, default to disclosure at time 0
        std::string disclosureString = disclosureExpression.empty() ? "0" : disclosureExpression;
        auto disclosureExpressionPtr = std::make_shared<Expression>(stochasticHandle, disclosureString, extensionElements->attributeRegistry);

        deferredAttributes[instanceId].push_back({attribute, node, initializationExpression, disclosureExpressionPtr});
      }
    }
  }

  // Finalize instances
  for (auto& [id, instance] : instances) {
    ensureDefaultValue(instance, Keyword::Instance, id);
    ensureDefaultValue(instance, Keyword::Timestamp);

    // Set placeholder instantiation time (actual value will be per-scenario)
    auto timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    instance.instantiation = instance.data.at(timestampAttribute);
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

BPMNOS::number StochasticDataProvider::getEffectiveDisclosure(size_t instanceId, const BPMN::Node* node, BPMNOS::number ownDisclosure) {
  BPMNOS::number effectiveDisclosure = ownDisclosure;

  if (auto childNode = node->represents<BPMN::ChildNode>()) {
    auto parentNode = childNode->parent;
    if (!disclosureTimes[instanceId].contains(parentNode)) {
      throw std::runtime_error("StochasticDataProvider: disclosure for '" + node->id + "' given before parent '" + parentNode->id + "'");
    }
    effectiveDisclosure = std::max(effectiveDisclosure, disclosureTimes[instanceId][parentNode]);
  }

  if (!disclosureTimes[instanceId].contains(node)) {
    disclosureTimes[instanceId][node] = effectiveDisclosure;
  }
  else {
    disclosureTimes[instanceId][node] = std::max(disclosureTimes[instanceId][node], effectiveDisclosure);
  }

  return effectiveDisclosure;
}

std::unique_ptr<Scenario> StochasticDataProvider::createScenario(unsigned int scenarioId) {
  auto scenario = std::make_unique<StochasticScenario>(model.get(), globalValueMap, seed + scenarioId);

  // Set references for expression evaluation
  scenario->randomFactory = &randomFactory;
  scenario->stochasticHandle = &stochasticHandle;

  for (auto& [id, instance] : instances) {
    scenario->addInstance(instance.process, id);
    for (auto& [attribute, value] : instance.data) {
      scenario->setValue(id, attribute, value);
    }
  }

  // Set node disclosure times (from immediate disclosures)
  for (auto& [instanceId, nodes] : disclosureTimes) {
    for (auto& [node, disclosureTime] : nodes) {
      scenario->setDisclosure(instanceId, node, disclosureTime);
    }
  }

  // Add deferred disclosures (shared expressions, evaluated per-scenario with different RNG)
  for (auto& [instanceId, deferreds] : deferredAttributes) {
    for (auto& deferred : deferreds) {
      scenario->addDeferredDisclosure(
        instanceId,
        {deferred.attribute, deferred.node, deferred.initializationExpression, deferred.disclosureExpression}
      );
    }
  }

  // Share completion expressions
  for (auto& [instanceId, tasks] : completionExpressions) {
    for (auto& [task, expressions] : tasks) {
      for (auto& expression : expressions) {
        scenario->addCompletionExpression(instanceId, task, expression);
      }
    }
  }

  // Share ready expressions
  for (auto& [instanceId, nodes] : readyExpressions) {
    for (auto& [node, expressions] : nodes) {
      for (auto& expression : expressions) {
        scenario->addReadyExpression(instanceId, node, expression);
      }
    }
  }

  // Evaluate deferred disclosures with scenario-specific RNG
  scenario->evaluateDeferredDisclosures();

  // Reveal data disclosed at time 0
  scenario->revealData(0);

  return scenario;
}
