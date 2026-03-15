#include "ExpectedValueDataProvider.h"
#include "ExpectedValueScenario.h"
#include "model/utility/src/Keywords.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <ranges>

using namespace BPMNOS::Model;

ExpectedValueDataProvider::ExpectedValueDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : ExpectedValueDataProvider(modelFile, {}, instanceFileOrString)
{
}

ExpectedValueDataProvider::ExpectedValueDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString)
  : StaticDataProvider(modelFile, folders)
{
  // Register expected value functions on the model's limexHandle
  expectedValueFactory.registerFunctions(model->limexHandle);

  reader = std::make_unique<CSVReader>(instanceFileOrString);
  readInstances();
}

void ExpectedValueDataProvider::readInstances() {
  CSVReader::Table table = reader->read();
  if (table.empty()) {
    throw std::runtime_error("ExpectedValueDataProvider: table is empty");
  }
  if (table.size() < 2) {
    throw std::runtime_error("ExpectedValueDataProvider: table must have at least a header and one data row");
  }

  size_t columnCount = table[0].size();
  if (columnCount == 3) {
    // Standard static format - delegate to base
    readInstancesNewFormat(table);
  }
  else if (columnCount == 4 || columnCount == 5) {
    // Extended format with DISCLOSURE and/or COMPLETION
    readInstancesExtendedFormat(table, columnCount);
  }
  else {
    throw std::runtime_error("ExpectedValueDataProvider: expected 3, 4, or 5 columns, got " + std::to_string(columnCount));
  }

  // Finalize instances
  for (auto& [id, instance] : instances) {
    ensureDefaultValue(instance, Keyword::Instance, id);
    ensureDefaultValue(instance, Keyword::Timestamp);
    instance.instantiation = instance.data.at(attributes[instance.process][Keyword::Timestamp]);

    if (earliestInstantiation > instance.instantiation) {
      earliestInstantiation = instance.instantiation;
    }
    if (latestInstantiation < instance.instantiation) {
      latestInstantiation = instance.instantiation;
    }
  }
}

void ExpectedValueDataProvider::readInstancesExtendedFormat(const CSVReader::Table& table, size_t columnCount) {
  enum { INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, COMPLETION };

  for (auto& row : table | std::views::drop(1)) {
    if (row.empty()) continue;
    if (row.size() != columnCount) {
      throw std::runtime_error("ExpectedValueDataProvider: inconsistent column count");
    }

    // Get instance ID
    if (!std::holds_alternative<std::string>(row.at(INSTANCE_ID))) {
      throw std::runtime_error("ExpectedValueDataProvider: illegal instance id");
    }
    std::string instanceIdStr = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID
    if (!std::holds_alternative<std::string>(row.at(NODE_ID))) {
      throw std::runtime_error("ExpectedValueDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization
    if (!std::holds_alternative<std::string>(row.at(INITIALIZATION))) {
      throw std::runtime_error("ExpectedValueDataProvider: illegal initialization");
    }
    std::string initialization = std::get<std::string>(row.at(INITIALIZATION));

    // DISCLOSURE column is ignored (index 3) - all data disclosed at time 0

    // Get completion expression if present (5-column format)
    std::string completionStr;
    if (columnCount == 5) {
      if (!std::holds_alternative<std::string>(row.at(COMPLETION))) {
        throw std::runtime_error("ExpectedValueDataProvider: illegal completion");
      }
      completionStr = std::get<std::string>(row.at(COMPLETION));
    }

    if (instanceIdStr.empty() && nodeId.empty()) {
      // Global attribute
      if (initialization.empty()) continue;
      auto [attributeName, expressionStr] = parseInitialization(initialization);

      const Attribute* attribute = nullptr;
      for (auto& [id, attr] : attributes[nullptr]) {
        if (attr->name == attributeName) {
          attribute = attr;
          break;
        }
      }
      if (!attribute) {
        throw std::runtime_error("ExpectedValueDataProvider: unknown global attribute '" + attributeName + "'");
      }

      Values globals(model->attributes.size());
      for (auto& [attr, value] : globalValueMap) {
        globals[attr->index] = value;
      }
      Expression expression(expressionStr, model->attributeRegistry);
      auto value = expression.execute(Values{}, Values{}, globals);
      if (!value.has_value()) {
        throw std::runtime_error("ExpectedValueDataProvider: failed to evaluate global '" + attributeName + "'");
      }
      globalValueMap[attribute] = value.value();
    }
    else if (instanceIdStr.empty()) {
      throw std::runtime_error("ExpectedValueDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdStr, STRING);
      BPMN::Node* node = findNode(nodeId);

      // First occurrence must be a process
      if (!instances.contains(instanceId)) {
        if (!node->represents<BPMN::Process>()) {
          throw std::runtime_error("ExpectedValueDataProvider: first row for instance must reference a process");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StaticInstanceData({process, instanceId, std::numeric_limits<BPMNOS::number>::max(), {}});
      }

      auto& instance = instances[instanceId];

      // Process initialization
      if (!initialization.empty()) {
        auto [attributeName, expressionStr] = parseInitialization(initialization);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attributeName)) {
          throw std::runtime_error("ExpectedValueDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
        }
        auto attribute = extensionElements->attributeRegistry[attributeName];
        if (attribute->expression) {
          throw std::runtime_error("ExpectedValueDataProvider: attribute '" + attributeName + "' is initialized by expression");
        }
        instance.data[attribute] = evaluateExpression(expressionStr);
      }

      // Process completion expression
      if (!completionStr.empty()) {
        auto [attributeName, expressionStr] = parseInitialization(completionStr);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attributeName)) {
          throw std::runtime_error("ExpectedValueDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
        }
        auto attribute = extensionElements->attributeRegistry[attributeName];
        completionExpressions[instanceId][node].push_back({attribute, expressionStr});
      }
    }
  }
}

void ExpectedValueDataProvider::initializeExpectedValueHandle() const {
  if (!expectedValueHandleInitialized) {
    const_cast<ExpectedValueFactory&>(expectedValueFactory).registerFunctions(expectedValueHandle);
    const_cast<bool&>(expectedValueHandleInitialized) = true;
  }
}

BPMNOS::number ExpectedValueDataProvider::evaluateExpressionWithExpectedValues(const std::string& expression) const {
  initializeExpectedValueHandle();
  LIMEX::Expression<double> compiled(expression, expectedValueHandle);
  if (!compiled.getVariables().empty() || !compiled.getCollections().empty()) {
    throw std::runtime_error("ExpectedValueDataProvider: expression must not reference variables");
  }
  return compiled.evaluate();
}

std::unique_ptr<Scenario> ExpectedValueDataProvider::createScenario([[maybe_unused]] unsigned int scenarioId) {
  auto scenario = std::make_unique<ExpectedValueScenario>(model.get(), earliestInstantiation, latestInstantiation, globalValueMap);

  // Add instances
  for (auto& [id, instance] : instances) {
    auto& timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    auto instantiationTime = instance.data[timestampAttribute];
    scenario->addInstance(instance.process, id, instantiationTime);
    for (auto& [attribute, value] : instance.data) {
      scenario->setValue(id, attribute, value);
    }
  }

  // Add completion expressions
  for (auto& [instanceId, nodeMap] : completionExpressions) {
    for (auto& [node, exprList] : nodeMap) {
      for (auto& exprData : exprList) {
        auto expression = std::make_unique<Expression>(exprData.expressionStr, model->attributeRegistry);
        scenario->addCompletionExpression(instanceId, node, {exprData.attribute, std::move(expression)});
      }
    }
  }

  return scenario;
}
