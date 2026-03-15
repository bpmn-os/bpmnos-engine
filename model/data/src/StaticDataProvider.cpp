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
  for ( auto& [ attributeId, attribute ] : attributes[nullptr] ) {
    if ( attribute->expression ) {
      Values globals(model->attributes.size());
      for ( auto& [attr, value] : globalValueMap ) {
        globals[attr->index] = value;
      }
      auto value = attribute->expression->execute(Values{}, Values{}, globals);
      if ( !value.has_value() ) {
        throw std::runtime_error("StaticDataProvider: failed to evaluate global attribute '" + attribute->id + "'");
      }
      globalValueMap[ attribute ] = value.value();
    }
  }
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
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
    std::string initialization = std::get<std::string>(row.at(INITIALIZATION));

    if (instanceIdentifier.empty() && nodeId.empty()) {
      // Global attribute
      if (initialization.empty()) {
        continue;
      }
      auto [attributeName, expressionString] = parseInitialization(initialization);
      // Find global attribute by name
      const Attribute* attribute = nullptr;
      for (auto& [id, globalAttribute] : attributes[nullptr]) {
        if (globalAttribute->name == attributeName) {
          attribute = globalAttribute;
          break;
        }
      }
      if (!attribute) {
        throw std::runtime_error("StaticDataProvider: unknown global attribute '" + attributeName + "'");
      }
      // Build globals vector from current globalValueMap
      Values globals(model->attributes.size());
      for (auto& [globalAttribute, value] : globalValueMap) {
        globals[globalAttribute->index] = value;
      }
      // Compile and evaluate using Expression
      Expression expression(expressionString, model->attributeRegistry);
      auto value = expression.execute(Values{}, Values{}, globals);
      if (!value.has_value()) {
        throw std::runtime_error("StaticDataProvider: failed to evaluate global '" + attributeName + "'");
      }
      globalValueMap[attribute] = value.value();
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
      if (initialization.empty()) {
        continue;
      }

      auto& instance = instances[instanceId];
      auto [attributeName, expressionString] = parseInitialization(initialization);

      // Look up attribute in the node's extension elements
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if (!extensionElements->attributeRegistry.contains(attributeName)) {
        throw std::runtime_error("StaticDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
      }

      auto attribute = extensionElements->attributeRegistry[attributeName];
      if (attribute->expression) {
        throw std::runtime_error("StaticDataProvider: value of attribute '" + attributeName + "' is initialized by expression and must not be provided explicitly");
      }

      instance.data[attribute] = evaluateExpression(expressionString);
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

std::pair<std::string, std::string> StaticDataProvider::parseInitialization(const std::string& initialization) const {
  // Parse "attributeName := expression"
  auto pos = initialization.find(":=");
  if ( pos == std::string::npos ) {
    throw std::runtime_error("StaticDataProvider: initialization must be in format 'attribute := expression', got '" + initialization + "'");
  }

  std::string attributeName = initialization.substr(0, pos);
  std::string expression = initialization.substr(pos + 2);

  // Trim whitespace
  auto trimStart = attributeName.find_first_not_of(" \t");
  auto trimEnd = attributeName.find_last_not_of(" \t");
  if ( trimStart == std::string::npos ) {
    throw std::runtime_error("StaticDataProvider: empty attribute name in initialization '" + initialization + "'");
  }
  attributeName = attributeName.substr(trimStart, trimEnd - trimStart + 1);

  trimStart = expression.find_first_not_of(" \t");
  trimEnd = expression.find_last_not_of(" \t");
  if ( trimStart == std::string::npos ) {
    throw std::runtime_error("StaticDataProvider: empty expression in initialization '" + initialization + "'");
  }
  expression = expression.substr(trimStart, trimEnd - trimStart + 1);

  return {attributeName, expression};
}

BPMNOS::number StaticDataProvider::evaluateExpression(const std::string& expressionString) const {
  Values globals(model->attributes.size());
  for (auto& [attribute, value] : globalValueMap) {
    globals[attribute->index] = value;
  }
  Expression expression(expressionString, model->attributeRegistry);
  for (auto* attribute : expression.variables) {
    if (attribute->category != Attribute::Category::GLOBAL) {
      throw std::runtime_error("StaticDataProvider: expression '" + expressionString +
                               "' references non-global attribute '" + attribute->name + "'");
    }
  }
  auto value = expression.execute(Values{}, Values{}, globals);
  if (!value.has_value()) {
    throw std::runtime_error("StaticDataProvider: failed to evaluate expression '" + expressionString + "'");
  }
  return value.value();
}

void StaticDataProvider::ensureDefaultValue(StaticInstanceData& instance, const std::string attributeId, std::optional<BPMNOS::number> value) {
  assert( attributes.contains(instance.process) );
  auto it1 = attributes.at(instance.process).find(attributeId);
  if ( it1 == attributes.at(instance.process).end() ) {
    throw std::runtime_error("StaticDataProvider: unable to find required attribute '" + attributeId + "' for process '" + instance.process->id + "'");
  }
  auto attribute = it1->second;
  if ( auto it2 = instance.data.find( attribute );
    it2 == instance.data.end()
  ) {
    if ( attribute->expression ) {
      throw std::runtime_error("StaticDataProvider: initial value of default attribute '" + attribute->id + "' must not be  provided by expression");
    }
    
    // set attribute value if available
    if ( value.has_value() ) {
      instance.data[ attribute ] = value.value();
    }
    else if ( attributeId == BPMNOS::Keyword::Timestamp ) {
      // use 0 as fallback 
      instance.data[ attribute ] = 0;
    }
    else {
      throw std::runtime_error("StaticDataProvider: attribute '" + attribute->id + "' has no default value");
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
