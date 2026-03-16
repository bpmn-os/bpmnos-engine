#include "DynamicDataProvider.h"
#include "DynamicScenario.h"
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

DynamicDataProvider::DynamicDataProvider(const std::string& modelFile, const std::string& instanceFileOrString)
  : DynamicDataProvider(modelFile,{},instanceFileOrString)
{
}

DynamicDataProvider::DynamicDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString)
  : DataProvider(modelFile,folders)
  , reader( CSVReader(instanceFileOrString) )
{
  readInstances();
}

void DynamicDataProvider::readInstances() {
  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("DynamicDataProvider: table '" + reader.instanceFileOrString + "' is empty");
  }

  if ( table.size() < 2 ) {
    throw std::runtime_error("DynamicDataProvider: table must have at least a header and one data row");
  }

  size_t columnCount = table[0].size();
  if ( columnCount != 4 ) {
    throw std::runtime_error("DynamicDataProvider: expected 4 columns (INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE), got " + std::to_string(columnCount));
  }

  // Parse table with format: INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE
  enum {INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE};

  for (auto &row : table | std::views::drop(1)) {   // assume a single header line
    if ( row.empty() ) {
      continue;
    }
    if ( row.size() != 4 ) {
      throw std::runtime_error("DynamicDataProvider: illegal number of cells");
    }

    // Get instance ID (may be empty for globals)
    if ( !std::holds_alternative<std::string>(row.at(INSTANCE_ID)) ) {
      throw std::runtime_error("DynamicDataProvider: illegal instance id");
    }
    std::string instanceIdentifier = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID (may be empty for globals)
    if ( !std::holds_alternative<std::string>(row.at(NODE_ID)) ) {
      throw std::runtime_error("DynamicDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization expression
    if ( !std::holds_alternative<std::string>(row.at(INITIALIZATION)) ) {
      throw std::runtime_error("DynamicDataProvider: illegal initialization");
    }
    std::string initializationString = std::get<std::string>(row.at(INITIALIZATION));

    // Get disclosure expression
    if ( !std::holds_alternative<std::string>(row.at(DISCLOSURE)) ) {
      throw std::runtime_error("DynamicDataProvider: illegal disclosure");
    }
    std::string disclosureString = std::get<std::string>(row.at(DISCLOSURE));

    if ( instanceIdentifier.empty() && nodeId.empty() ) {
      // Global attribute
      if ( !disclosureString.empty() ) {
        throw std::runtime_error("DynamicDataProvider: global attributes must not have disclosure expression");
      }
      if ( initializationString.empty() ) {
        continue;
      }
      auto [attributeName, expressionString] = DataProvider::parseInitialization(initializationString);
      // Find global attribute by name
      const Attribute* attribute = nullptr;
      for ( auto& [id, globalAttribute] : attributes[nullptr] ) {
        if ( globalAttribute->name == attributeName ) {
          attribute = globalAttribute;
          break;
        }
      }
      if ( !attribute ) {
        throw std::runtime_error("DynamicDataProvider: unknown global attribute '" + attributeName + "'");
      }
      // Build globals vector from current globalValueMap
      Values globals(model->attributes.size());
      for ( auto& [globalAttribute, value] : globalValueMap ) {
        globals[globalAttribute->index] = value;
      }
      // Compile and evaluate using Expression
      Expression expression(expressionString, model->attributeRegistry);
      auto value = expression.execute(Values{}, Values{}, globals);
      if ( !value.has_value() ) {
        throw std::runtime_error("DynamicDataProvider: failed to evaluate global '" + attributeName + "'");
      }
      globalValueMap[attribute] = value.value();
    }
    else if ( instanceIdentifier.empty() ) {
      throw std::runtime_error("DynamicDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdentifier, STRING);

      // Find the node
      BPMN::Node* node = findNode(nodeId);

      // First occurrence of instance must have node = process
      if ( !instances.contains(instanceId) ) {
        // Check that node is a process
        if ( !node->represents<BPMN::Process>() ) {
          throw std::runtime_error("DynamicDataProvider: first row for instance '" + instanceIdentifier + "' must reference a process node, got '" + nodeId + "'");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = DynamicInstanceData({process, instanceId, std::numeric_limits<BPMNOS::number>::max(), {}});
        // Initialize process disclosure to 0 (may be updated later if process attributes have disclosure)
        disclosure[instanceId][process] = 0;
      }

      // If no initialization, just create instance (already done above)
      if ( initializationString.empty() ) {
        continue;
      }

      auto& instance = instances[instanceId];
      auto [attributeName, expressionString] = DataProvider::parseInitialization(initializationString);

      // Look up attribute in the node's extension elements
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if ( !extensionElements->attributeRegistry.contains(attributeName) ) {
        throw std::runtime_error("DynamicDataProvider: node '" + nodeId + "' has no attribute '" + attributeName + "'");
      }

      auto attribute = extensionElements->attributeRegistry[attributeName];
      if ( attribute->expression ) {
        throw std::runtime_error("DynamicDataProvider: value of attribute '" + attributeName + "' is initialized by expression and must not be provided explicitly");
      }

      // Parse disclosure time (must be constant)
      BPMNOS::number ownDisclosure = 0;
      if ( !disclosureString.empty() ) {
        ownDisclosure = evaluateExpression(disclosureString);
      }

      // Compute effective disclosure = max(own, parent_scope_disclosure)
      BPMNOS::number disclosureTime = getEffectiveDisclosure(instanceId, node, ownDisclosure);

      // Evaluate expression at parse time (globals only)
      BPMNOS::number value = evaluateExpression(expressionString);

      if (disclosureTime == 0) {
        // Immediate disclosure: store value directly
        instance.data[attribute] = value;
      }
      else {
        // Deferred disclosure: store pre-computed value for later reveal
        deferredInitializations[instanceId].push_back({attribute, disclosureTime, value});
      }
    }
  }

  for (auto& [id, instance] : instances) {
    // ensure that default attributes are available
    ensureDefaultValue(instance, Keyword::Instance, id);
    ensureDefaultValue(instance, Keyword::Timestamp);
    // set time of instantiation
    instance.instantiation = instance.data.at( attributes[instance.process][Keyword::Timestamp] );

    // Effective instantiation time is max(instantiation, processDisclosure)
    BPMNOS::number effectiveInstantiation = instance.instantiation;
    if ( disclosure.contains(id) && disclosure.at(id).contains(instance.process) ) {
      effectiveInstantiation = std::max(effectiveInstantiation, disclosure.at(id).at(instance.process));
    }

    if ( earliestInstantiation > effectiveInstantiation ) {
      earliestInstantiation = effectiveInstantiation;
    }
    if ( latestInstantiation < effectiveInstantiation ) {
      latestInstantiation = effectiveInstantiation;
    }
  }
}

BPMNOS::number DynamicDataProvider::getEffectiveDisclosure(size_t instanceId, const BPMN::Node* node, BPMNOS::number ownDisclosure) {
  BPMNOS::number effectiveDisclosure = ownDisclosure;

  // Check parent scope's disclosure time
  if ( auto childNode = node->represents<BPMN::ChildNode>() ) {
    auto parentNode = childNode->parent;
    if ( !disclosure[instanceId].contains(parentNode) ) {
      throw std::runtime_error("DynamicDataProvider: disclosure for '" + node->id + "' given before parent '" + parentNode->id + "'");
    }
    effectiveDisclosure = std::max(effectiveDisclosure, disclosure[instanceId][parentNode]);
  }

  // Update this scope's disclosure time (track maximum seen for this scope)
  if ( !disclosure[instanceId].contains(node) ) {
    disclosure[instanceId][node] = effectiveDisclosure;
  }
  else {
    disclosure[instanceId][node] = std::max(disclosure[instanceId][node], effectiveDisclosure);
  }

  return effectiveDisclosure;
}

std::unique_ptr<Scenario> DynamicDataProvider::createScenario([[maybe_unused]] unsigned int scenarioId) {
  auto scenario = std::make_unique<DynamicScenario>(model.get(), earliestInstantiation, latestInstantiation, globalValueMap);
  for ( auto& [id, instance] : instances ) {
    auto& timestampAttribute = attributes[instance.process][Keyword::Timestamp];
    auto instantiationTime = instance.data[timestampAttribute];
    scenario->addInstance(instance.process, id, instantiationTime);
    for ( auto& [attribute, value] : instance.data ) {
      scenario->setValue(id, attribute, value);
    }
  }
  // Set node disclosure times (effective disclosure = max of own and parent scope)
  for ( auto& [instanceId, nodes] : disclosure ) {
    for ( auto& [node, disclosureTime] : nodes ) {
      scenario->setDisclosure(instanceId, node, disclosureTime);
    }
  }
  // Add deferred initializations (pre-computed values)
  for (auto& [instanceId, pendings] : deferredInitializations) {
    for (auto& pending : pendings) {
      scenario->addPendingDisclosure(instanceId, {pending.attribute, pending.disclosureTime, pending.value});
    }
  }
  // Reveal data disclosed at time 0
  scenario->revealData(0);
  return scenario;
}
