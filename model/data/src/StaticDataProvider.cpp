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

using namespace BPMNOS::Model;

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : StaticDataProvider(modelFile,{},instanceFileOrString)
{
}

StaticDataProvider::StaticDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString)
  : DataProvider(modelFile,folders)
  , reader( CSVReader(instanceFileOrString) )
{
  for ( auto& [ attributeId, attribute ] : attributes[nullptr] ) {
    if ( attribute->expression ) {
      if ( attribute->expression->compiled.getVariables().size() || attribute->expression->compiled.getCollections().size() ) {
        throw std::runtime_error("StaticDataProvider: initial value of global attribute '" + attribute->id + "' must not be derived from other attributes");
      }
      globalValueMap[ attribute ] = attribute->expression->compiled.evaluate();
    }
  }
  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
  readInstances();
  
}

void StaticDataProvider::readInstances() {
  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("StaticDataProvider: table '" + reader.instanceFileOrString + "' is empty");
  }

  // Detect format by column count of first data row
  if ( table.size() < 2 ) {
    throw std::runtime_error("StaticDataProvider: table must have at least a header and one data row");
  }

  size_t columnCount = table[0].size();
  if ( columnCount == 4 ) {
    // Old format: PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE
    readInstancesOldFormat(table);
  }
  else if ( columnCount == 3 ) {
    // New format: INSTANCE_ID, NODE_ID, INITIALIZATION
    readInstancesNewFormat(table);
  }
  else {
    throw std::runtime_error("StaticDataProvider: expected 3 or 4 columns, got " + std::to_string(columnCount));
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

void StaticDataProvider::readInstancesOldFormat(const CSVReader::Table& table) {
  enum {PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE};

  // Show deprecation warning
  std::cerr << "WARNING: Old CSV format (PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE) is deprecated.\n";
  std::cerr << "         Please migrate to new format (INSTANCE_ID, NODE_ID, INITIALIZATION).\n";

  for (auto &row : table | std::views::drop(1)) {   // assume a single header line
    if ( row.empty() ) {
      continue;
    }
    if ( row.size() != 4 ) {
      throw std::runtime_error("StaticDataProvider: illegal number of cells");
    }
    if ( !std::holds_alternative<std::string>(row.at(PROCESS_ID)) ) {
      throw std::runtime_error("StaticDataProvider: illegal process id");
    }
    std::string processId = std::get<std::string>(row.at(PROCESS_ID));

    if ( processId.size() ) {
      // find process with respective identifier
      auto processIt = std::find_if(
        model->processes.begin(),
        model->processes.end(),
        [&processId](const std::unique_ptr<BPMN::Process>& process) { return process->id == processId;}
      );
      if ( processIt == model->processes.end() ) {
        throw std::runtime_error("StaticDataProvider: model has no process '" + processId + "'");
      }

      auto process = processIt->get();

      if ( !std::holds_alternative<std::string>(row.at(INSTANCE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal instance id");
      }
      auto instanceId = (size_t)BPMNOS::to_number(std::get<std::string>(row.at(INSTANCE_ID)), STRING );
      // find instance with respective identifier
      if ( !instances.contains(instanceId) ) {
        // row has first entry for instance, create new entry in data
        instances[instanceId] = StaticInstanceData({process,instanceId,std::numeric_limits<BPMNOS::number>::max(),{}});
      }

      auto& instance = instances[instanceId];

      if ( !std::holds_alternative<std::string>(row.at(ATTRIBUTE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal attribute id");
      }
      std::string attributeId = std::get<std::string>(row.at(ATTRIBUTE_ID));

      if ( attributeId == "" ) {
        // no attribute provided in this row
        continue;
      }

      if ( !attributes[process].contains(attributeId) ) {
        throw std::runtime_error("StaticDataProvider: process '" + processId + "' has no node with attribute '" + attributeId + "'");
      }

      auto attribute = attributes[process][attributeId];
      if ( attribute->expression ) {
        throw std::runtime_error("StaticDataProvider: value of attribute '" + attributeId + "' is initialized by expression and must not be provided explicitly");
      }

      if ( !std::holds_alternative<BPMNOS::number>(row.at(VALUE)) ) {
        throw std::runtime_error("StaticDataProvider: illegal value");
      }
      instance.data[ attribute ] = std::get<BPMNOS::number>(row.at(VALUE));
    }
    else {
      // row contains global attribute
      if ( !std::holds_alternative<std::string>(row.at(ATTRIBUTE_ID)) ) {
        throw std::runtime_error("StaticDataProvider: illegal attribute id");
      }
      std::string attributeId = std::get<std::string>(row.at(ATTRIBUTE_ID));
      auto attribute = attributes[nullptr][attributeId];
      if ( !std::holds_alternative<BPMNOS::number>(row.at(VALUE)) ) {
        throw std::runtime_error("StaticDataProvider: illegal value");
      }
      globalValueMap[attribute] = std::get<BPMNOS::number>(row.at(VALUE));
    }
  }
}

void StaticDataProvider::readInstancesNewFormat(const CSVReader::Table& table) {
  enum {INSTANCE_ID, NODE_ID, INITIALIZATION};

  for (auto &row : table | std::views::drop(1)) {   // assume a single header line
    if ( row.empty() ) {
      continue;
    }
    if ( row.size() != 3 ) {
      throw std::runtime_error("StaticDataProvider: illegal number of cells");
    }

    // Get instance ID (may be empty for globals)
    if ( !std::holds_alternative<std::string>(row.at(INSTANCE_ID)) ) {
      throw std::runtime_error("StaticDataProvider: illegal instance id");
    }
    std::string instanceIdStr = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID (may be empty for globals)
    if ( !std::holds_alternative<std::string>(row.at(NODE_ID)) ) {
      throw std::runtime_error("StaticDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization expression
    if ( !std::holds_alternative<std::string>(row.at(INITIALIZATION)) ) {
      throw std::runtime_error("StaticDataProvider: illegal initialization");
    }
    std::string initialization = std::get<std::string>(row.at(INITIALIZATION));

    if ( initialization.empty() ) {
      continue;
    }

    // Parse "attr := expr" to get attribute name and value
    auto [attributeName, value] = parseInitialization(initialization);

    if ( instanceIdStr.empty() && nodeId.empty() ) {
      // Global attribute
      if ( !attributes[nullptr].contains(attributeName) ) {
        throw std::runtime_error("StaticDataProvider: unknown global attribute '" + attributeName + "'");
      }
      auto attribute = attributes[nullptr][attributeName];
      globalValueMap[attribute] = value;
    }
    else if ( instanceIdStr.empty() ) {
      throw std::runtime_error("StaticDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdStr, STRING);

      // Find the node
      BPMN::Node* node = findNode(nodeId);

      // First occurrence of instance must have node = process
      if ( !instances.contains(instanceId) ) {
        // Check that node is a process
        if ( !node->represents<BPMN::Process>() ) {
          throw std::runtime_error("StaticDataProvider: first row for instance '" + instanceIdStr + "' must reference a process node, got '" + nodeId + "'");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StaticInstanceData({process, instanceId, std::numeric_limits<BPMNOS::number>::max(), {}});
      }

      auto& instance = instances[instanceId];

      // Look up attribute in the node's extension elements
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if ( !extensionElements->attributeRegistry.contains(attributeName) ) {
        throw std::runtime_error("StaticDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
      }

      auto attribute = extensionElements->attributeRegistry[attributeName];
      if ( attribute->expression ) {
        throw std::runtime_error("StaticDataProvider: value of attribute '" + attributeName + "' is initialized by expression and must not be provided explicitly");
      }

      instance.data[attribute] = value;
    }
  }
}

std::pair<std::string, BPMNOS::number> StaticDataProvider::parseInitialization(const std::string& initialization) const {
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

  // Evaluate expression using LIMEX
  LIMEX::Expression<double> compiled(expression, model->limexHandle);
  if ( !compiled.getVariables().empty() || !compiled.getCollections().empty() ) {
    throw std::runtime_error("StaticDataProvider: initialization expression must not reference variables, got '" + expression + "'");
  }
  BPMNOS::number value = compiled.evaluate();

  return {attributeName, value};
}

std::string StaticDataProvider::convertToNewFormat(const CSVReader::Table& table) const {
  // TODO: implement conversion from old to new format
  return "";
}

void StaticDataProvider::promptMigration(const std::string& newFormatContent) const {
  // TODO: implement migration prompt
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
