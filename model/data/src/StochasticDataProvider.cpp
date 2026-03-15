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

  // Evaluate global attributes
  for (auto& [attributeId, attribute] : attributes[nullptr]) {
    if (attribute->expression) {
      Values globals(model->attributes.size());
      for (auto& [globalAttribute, globalValue] : globalValueMap) {
        globals[globalAttribute->index] = globalValue;
      }
      auto value = attribute->expression->execute(Values{}, Values{}, globals);
      if (!value.has_value()) {
        throw std::runtime_error("StochasticDataProvider: failed to evaluate global attribute '" + attribute->id + "'");
      }
      globalValueMap[attribute] = value.value();
    }
  }

  earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();
  latestInstantiation = std::numeric_limits<BPMNOS::number>::min();
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
  if (columnCount < 3 || columnCount > 5) {
    throw std::runtime_error("StochasticDataProvider: expected 3-5 columns, got " + std::to_string(columnCount));
  }

  // Column indices
  enum { INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, COMPLETION };

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
    std::string instanceIdStr = std::get<std::string>(row.at(INSTANCE_ID));

    // Get node ID
    if (!std::holds_alternative<std::string>(row.at(NODE_ID))) {
      throw std::runtime_error("StochasticDataProvider: illegal node id");
    }
    std::string nodeId = std::get<std::string>(row.at(NODE_ID));

    // Get initialization expression
    if (!std::holds_alternative<std::string>(row.at(INITIALIZATION))) {
      throw std::runtime_error("StochasticDataProvider: illegal initialization");
    }
    std::string initialization = std::get<std::string>(row.at(INITIALIZATION));

    // Get disclosure (if 4+ columns)
    std::string disclosureStr;
    if (columnCount >= 4) {
      if (!std::holds_alternative<std::string>(row.at(DISCLOSURE))) {
        throw std::runtime_error("StochasticDataProvider: illegal disclosure");
      }
      disclosureStr = std::get<std::string>(row.at(DISCLOSURE));
    }

    // Get completion (if 5 columns)
    std::string completionStr;
    if (columnCount >= 5) {
      if (!std::holds_alternative<std::string>(row.at(COMPLETION))) {
        throw std::runtime_error("StochasticDataProvider: illegal completion");
      }
      completionStr = std::get<std::string>(row.at(COMPLETION));
    }

    // Handle global attributes
    if (instanceIdStr.empty() && nodeId.empty()) {
      if (!disclosureStr.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have disclosure");
      }
      if (!completionStr.empty()) {
        throw std::runtime_error("StochasticDataProvider: global attributes must not have completion");
      }
      if (initialization.empty()) {
        continue;
      }

      auto [attributeName, expressionStr] = parseInitialization(initialization);
      const Attribute* attribute = nullptr;
      for (auto& [id, globalAttribute] : attributes[nullptr]) {
        if (globalAttribute->name == attributeName) {
          attribute = globalAttribute;
          break;
        }
      }
      if (!attribute) {
        throw std::runtime_error("StochasticDataProvider: unknown global attribute '" + attributeName + "'");
      }

      Values globals(model->attributes.size());
      for (auto& [globalAttribute, globalValue] : globalValueMap) {
        globals[globalAttribute->index] = globalValue;
      }

      Expression expression(stochasticHandle, expressionStr, model->attributeRegistry);
      auto value = expression.execute(Values{}, Values{}, globals);
      if (!value.has_value()) {
        throw std::runtime_error("StochasticDataProvider: failed to evaluate global '" + attributeName + "'");
      }
      globalValueMap[attribute] = value.value();
    }
    else if (instanceIdStr.empty()) {
      throw std::runtime_error("StochasticDataProvider: instance id required when node id is provided");
    }
    else {
      auto instanceId = (size_t)BPMNOS::to_number(instanceIdStr, STRING);
      BPMN::Node* node = findNode(nodeId);

      // First occurrence of instance must have node = process
      if (!instances.contains(instanceId)) {
        if (!node->represents<BPMN::Process>()) {
          throw std::runtime_error("StochasticDataProvider: first row for instance '" + instanceIdStr +
                                   "' must reference a process node, got '" + nodeId + "'");
        }
        auto process = dynamic_cast<BPMN::Process*>(node);
        instances[instanceId] = StochasticInstanceData{process, instanceId,
                                                       std::numeric_limits<BPMNOS::number>::max(), {}};
        disclosure[instanceId][process] = 0;
      }

      auto& instance = instances[instanceId];

      // Handle COMPLETION expression (only valid for Tasks, not SendTask/ReceiveTask/DecisionTask)
      if (!completionStr.empty()) {
        if (!node->represents<BPMN::Task>() ||
            node->represents<BPMN::SendTask>() ||
            node->represents<BPMN::ReceiveTask>() ||
            node->represents<DecisionTask>()) {
          throw std::runtime_error("StochasticDataProvider: COMPLETION only valid for Task nodes, not '" +
                                   nodeId + "'");
        }

        auto [attrName, exprStr] = parseInitialization(completionStr);
        auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        if (!extensionElements->attributeRegistry.contains(attrName)) {
          throw std::runtime_error("StochasticDataProvider: node '" + nodeId +
                                   "' has no attribute '" + attrName + "'");
        }

        auto attribute = extensionElements->attributeRegistry[attrName];
        auto expression = std::make_unique<Expression>(stochasticHandle, exprStr,
                                                       extensionElements->attributeRegistry);
        completionExpressions[instanceId][node].push_back({attribute, std::move(expression)});
      }

      // Handle INITIALIZATION
      if (initialization.empty()) {
        continue;
      }

      auto [attributeName, expressionStr] = parseInitialization(initialization);
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if (!extensionElements->attributeRegistry.contains(attributeName)) {
        throw std::runtime_error("StochasticDataProvider: node '" + nodeId +
                                 "' has no attribute '" + attributeName + "'");
      }

      auto attribute = extensionElements->attributeRegistry[attributeName];
      if (attribute->expression) {
        throw std::runtime_error("StochasticDataProvider: value of attribute '" + attributeName +
                                 "' is initialized by expression and must not be provided explicitly");
      }

      // Parse disclosure time
      BPMNOS::number ownDisclosure = 0;
      if (!disclosureStr.empty()) {
        ownDisclosure = evaluateExpression(disclosureStr);
      }

      BPMNOS::number disclosureTime = getEffectiveDisclosure(instanceId, node, ownDisclosure);

      if (disclosureTime == 0) {
        // Immediate disclosure: evaluate now
        instance.data[attribute] = evaluateExpression(expressionStr);
      }
      else {
        // Deferred disclosure: store expression for later evaluation
        auto expression = std::make_unique<Expression>(stochasticHandle, expressionStr,
                                                       extensionElements->attributeRegistry);
        pendingDisclosures[instanceId].push_back({attribute, disclosureTime, std::move(expression)});
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

std::pair<std::string, std::string> StochasticDataProvider::parseInitialization(
    const std::string& initialization) const {
  auto pos = initialization.find(":=");
  if (pos == std::string::npos) {
    throw std::runtime_error("StochasticDataProvider: initialization must be 'attribute := expression', got '" +
                             initialization + "'");
  }

  std::string attributeName = initialization.substr(0, pos);
  std::string expression = initialization.substr(pos + 2);

  auto trimStart = attributeName.find_first_not_of(" \t");
  auto trimEnd = attributeName.find_last_not_of(" \t");
  if (trimStart == std::string::npos) {
    throw std::runtime_error("StochasticDataProvider: empty attribute name in '" + initialization + "'");
  }
  attributeName = attributeName.substr(trimStart, trimEnd - trimStart + 1);

  trimStart = expression.find_first_not_of(" \t");
  trimEnd = expression.find_last_not_of(" \t");
  if (trimStart == std::string::npos) {
    throw std::runtime_error("StochasticDataProvider: empty expression in '" + initialization + "'");
  }
  expression = expression.substr(trimStart, trimEnd - trimStart + 1);

  return {attributeName, expression};
}

BPMNOS::number StochasticDataProvider::evaluateExpression(const std::string& expression) const {
  LIMEX::Expression<double> compiled(expression, stochasticHandle);
  if (!compiled.getVariables().empty() || !compiled.getCollections().empty()) {
    throw std::runtime_error("StochasticDataProvider: expression must not reference variables, got '" +
                             expression + "'");
  }
  return compiled.evaluate();
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

void StochasticDataProvider::ensureDefaultValue(StochasticInstanceData& instance,
                                                 const std::string attributeId,
                                                 std::optional<BPMNOS::number> value) {
  assert(attributes.contains(instance.process));
  auto it1 = attributes.at(instance.process).find(attributeId);
  if (it1 == attributes.at(instance.process).end()) {
    throw std::runtime_error("StochasticDataProvider: unable to find required attribute '" +
                             attributeId + "' for process '" + instance.process->id + "'");
  }

  auto attribute = it1->second;
  if (auto it2 = instance.data.find(attribute); it2 == instance.data.end()) {
    if (attribute->expression) {
      throw std::runtime_error("StochasticDataProvider: initial value of default attribute '" +
                               attribute->id + "' must not be provided by expression");
    }

    if (value.has_value()) {
      instance.data[attribute] = value.value();
    }
    else if (attributeId == BPMNOS::Keyword::Timestamp) {
      instance.data[attribute] = 0;
    }
    else {
      throw std::runtime_error("StochasticDataProvider: attribute '" + attribute->id + "' has no default value");
    }
  }
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

  // Add pending disclosures
  for (auto& [instanceId, pendings] : pendingDisclosures) {
    for (auto& pending : pendings) {
      // Need to copy the expression since scenario takes ownership
      auto expression = std::make_unique<Expression>(stochasticHandle, pending.expression->expression,
                                                     pending.expression->attributeRegistry);
      scenario->addPendingDisclosure(instanceId, {pending.attribute, pending.disclosureTime,
                                                   std::move(expression)});
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

  // Reveal data disclosed at time 0
  scenario->revealData(0);

  return scenario;
}
