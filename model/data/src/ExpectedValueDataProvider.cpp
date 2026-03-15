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
  initializeExpectedValueHandle();

  reader = std::make_unique<CSVReader>(instanceFileOrString);
  readInstances();
}

void ExpectedValueDataProvider::initializeExpectedValueHandle() {
  // Copy lookup tables from model
  for (auto& lookupTable : model->lookupTables) {
    auto* table = lookupTable.get();
    expectedValueHandle.add(table->name, [table](const std::vector<double>& args) {
      return table->at(args);
    });
  }

  // Register expected value functions
  expectedValueFactory.registerFunctions(expectedValueHandle);
}

BPMNOS::number ExpectedValueDataProvider::evaluateExpression(const std::string& expressionString) const {
  Values globals(model->attributes.size());
  for (auto& [attribute, value] : globalValueMap) {
    globals[attribute->index] = value;
  }
  Expression expression(expectedValueHandle, expressionString, model->attributeRegistry);
  for (auto* attribute : expression.variables) {
    if (attribute->category != Attribute::Category::GLOBAL) {
      throw std::runtime_error("ExpectedValueDataProvider: expression '" + expressionString +
                               "' references non-global attribute '" + attribute->name + "'");
    }
  }
  auto value = expression.execute(Values{}, Values{}, globals);
  if (!value.has_value()) {
    throw std::runtime_error("ExpectedValueDataProvider: failed to evaluate expression '" + expressionString + "'");
  }
  return value.value();
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
  else if (columnCount == 4 || columnCount == 6) {
    // Extended format with DISCLOSURE (4) or DISCLOSURE/ARRIVAL/COMPLETION (6)
    readInstancesExtendedFormat(table, columnCount);
  }
  else {
    throw std::runtime_error("ExpectedValueDataProvider: expected 3, 4, or 6 columns, got " + std::to_string(columnCount));
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
  // Column indices: INSTANCE_ID(0), NODE_ID(1), INITIALIZATION(2), DISCLOSURE(3), [ARRIVAL(4), COMPLETION(5)]
  enum { INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, ARRIVAL, COMPLETION };

  for (auto& row : table | std::views::drop(1)) {
    if (row.empty()) continue;
    if (row.size() != columnCount) {
      throw std::runtime_error("ExpectedValueDataProvider: inconsistent column count");
    }

    // Get instance ID
    if (!std::holds_alternative<std::string>(row.at(INSTANCE_ID))) {
      throw std::runtime_error("ExpectedValueDataProvider: illegal instance id");
    }
    std::string instanceIdentifier = std::get<std::string>(row.at(INSTANCE_ID));

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
    // ARRIVAL column is ignored (index 4) - not applicable for expected value computation
    // COMPLETION column is ignored (index 5) - operators can be used to compute expected values

    if (instanceIdentifier.empty() && nodeId.empty()) {
      // Global attribute
      if (initialization.empty()) continue;
      auto [attributeName, expressionString] = parseInitialization(initialization);

      const Attribute* attribute = nullptr;
      for (auto& [id, globalAttribute] : attributes[nullptr]) {
        if (globalAttribute->name == attributeName) {
          attribute = globalAttribute;
          break;
        }
      }
      if (!attribute) {
        throw std::runtime_error("ExpectedValueDataProvider: unknown global attribute '" + attributeName + "'");
      }

      Values globals(model->attributes.size());
      for (auto& [globalAttribute, value] : globalValueMap) {
        globals[globalAttribute->index] = value;
      }
      Expression expression(expectedValueHandle, expressionString, model->attributeRegistry);
      auto value = expression.execute(Values{}, Values{}, globals);
      if (!value.has_value()) {
        throw std::runtime_error("ExpectedValueDataProvider: failed to evaluate global '" + attributeName + "'");
      }
      globalValueMap[attribute] = value.value();
    }
    else if (instanceIdentifier.empty()) {
      throw std::runtime_error("ExpectedValueDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdentifier, STRING);
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
        auto [attributeName, expressionString] = parseInitialization(initialization);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attributeName)) {
          throw std::runtime_error("ExpectedValueDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
        }
        auto attribute = extensionElements->attributeRegistry[attributeName];
        if (attribute->expression) {
          throw std::runtime_error("ExpectedValueDataProvider: attribute '" + attributeName + "' is initialized by expression");
        }
        instance.data[attribute] = evaluateExpression(expressionString);
      }
    }
  }
}

std::unique_ptr<Scenario> ExpectedValueDataProvider::createScenario([[maybe_unused]] unsigned int scenarioId) {
  // ExpectedValueScenario is identical to StaticScenario - just returns pre-computed values
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

  return scenario;
}
