#include "StaticDataProvider.h"
#include "StaticScenario.h"
#include "model/utility/src/Keywords.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/getDelimiter.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <unordered_map>
#include <algorithm>
#include <ranges>
#include <set>
#include <sstream>
#include <fstream>

using namespace BPMNOS::Model;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : StaticDataProvider(modelFile,{},instanceFileOrString)
{
}

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::vector<std::string>& folders)
  : DataProvider(modelFile, folders)
{
}

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString)
  : StaticDataProvider(modelFile, folders)
{
  reader = std::make_unique<CSVReader>(instanceFileOrString);
  readInstances();
}

void StaticDataProvider::readInstances() {
  CSVReader::Table table = reader->read();
  if (table.empty()) {
    throw std::runtime_error("StaticDataProvider: table '" + reader->instanceFileOrString + "' is empty");
  }

  if (table.size() < 2) {
    throw std::runtime_error("StaticDataProvider: table must have at least a header and one data row");
  }

  size_t columnCount = table[0].size();
  if (columnCount != 3) {
    throw std::runtime_error("StaticDataProvider: expected 3 columns (INSTANCE_ID, NODE_ID, INITIALIZATION), got " + std::to_string(columnCount));
  }

  enum { INSTANCE_ID, NODE_ID, INITIALIZATION };

  for (auto& row : table | std::views::drop(1)) {
    if (row.empty()) {
      continue;
    }
    if (row.size() != 3) {
      throw std::runtime_error("StaticDataProvider: inconsistent number of cells");
    }

    // Get instance ID (may be empty for globals)
    if (!std::holds_alternative<std::string>(row.at(INSTANCE_ID))) {
      throw std::runtime_error("StaticDataProvider: illegal instance id");
    }
    std::string instanceIdentifier = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID (may be empty for globals)
    if (!std::holds_alternative<std::string>(row.at(NODE_ID))) {
      throw std::runtime_error("StaticDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization expression
    if (!std::holds_alternative<std::string>(row.at(INITIALIZATION))) {
      throw std::runtime_error("StaticDataProvider: illegal initialization");
    }
    std::string initializationString = std::get<std::string>(row.at(INITIALIZATION));

    if (instanceIdentifier.empty() && nodeId.empty()) {
      // Global attribute
      if (initializationString.empty()) {
        continue;
      }
      evaluateGlobal(initializationString);
    }
    else if (instanceIdentifier.empty()) {
      throw std::runtime_error("StaticDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdentifier, STRING);

      // Find the node
      BPMN::Node* node = findNode(nodeId);

      // First occurrence of instance must have node = process
      if (!instances.contains(instanceId)) {
        // Check that node is a process
        if (!node->represents<BPMN::Process>()) {
          throw std::runtime_error("StaticDataProvider: first row for instance '" + instanceIdentifier + "' must reference a process node, got '" + nodeId + "'");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StaticInstanceData({process, instanceId, std::numeric_limits<BPMNOS::number>::max(), {}});
      }

      // If no initialization, just create instance (already done above)
      if (initializationString.empty()) {
        continue;
      }

      auto& instance = instances[instanceId];
      auto [attribute, expressionString] = lookupAttribute(node, initializationString);

      BPMNOS::number value = evaluateExpression(instanceId, node, expressionString, attribute->type);
      instance.data[attribute] = value;
      parseTimeEvaluatedValues[instanceId][attribute] = value;
    }
  }

  for (auto& [id, instance] : instances) {
    // ensure that default attributes are available
    ensureDefaultValue(instance, Keyword::Instance, id);
    ensureDefaultValue(instance, Keyword::Timestamp);
    // set time of instantiation
    instance.instantiation = instance.data.at( attributes[instance.process][Keyword::Timestamp] );

    if ( earliestInstantiation > instance.instantiation ) {
      earliestInstantiation = instance.instantiation;
    }
    if ( latestInstantiation < instance.instantiation ) {
      latestInstantiation = instance.instantiation;
    }
  }
}

std::unique_ptr<Scenario> StaticDataProvider::createScenario([[maybe_unused]] unsigned int scenarioId) {
  auto scenario = std::make_unique<StaticScenario>(model.get(), earliestInstantiation, latestInstantiation, globalValueMap);
  for ( auto& [id, instance] : instances ) {
    auto& timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    auto instantiationTime = instance.data[timestampAttribute];
    scenario->addInstance(instance.process, id, instantiationTime);
    for ( auto& [attribute, value] : instance.data ) {
      scenario->setValue(id, attribute, value);
    }
  }
  return scenario;
}
