#include "DataProvider.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

using namespace BPMNOS::Model;

DataProvider::DataProvider(const std::string& modelFile, const std::vector<std::string>& folders)
  : model(std::make_unique<Model>(modelFile, folders))
{
  // determine all global attributes
  attributes[nullptr] = {};
  for ( auto& attribute : model->attributes ) {
    attributes[nullptr].emplace(attribute->id,attribute.get());
  }

  // determine all attributes of all processes
  for ( auto& process : model->processes ) {
    // get all nodes in process with attribute definition
    std::vector< BPMN::Node* > nodes = process->find_all(
      [](BPMN::Node* node) {
        if ( node->extensionElements ) {
          if ( node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
            return true;
          }
        }
        return false;
      }
    );

    attributes[process.get()] = {};
    // add all attributes of process
    for ( auto& node : nodes ) {
      auto extensionElements = node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      // add all status attributes
      for ( auto& attribute : extensionElements->attributes ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
      // add all data attributes
      for ( auto& attribute : extensionElements->data ) {
        attributes[process.get()].emplace(attribute->id,attribute.get());
      }
      
      // add all guiding attributes
      if ( extensionElements->entryGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->entryGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->exitGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->exitGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->choiceGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->choiceGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
      if ( extensionElements->messageDeliveryGuidance.has_value() ) {
        for ( auto& attribute : extensionElements->messageDeliveryGuidance.value()->attributes ) {
          attributes[process.get()].emplace(attribute->id,attribute.get());
        }
      }
    }
  }

}

DataProvider::~DataProvider() {}

const Model& DataProvider::getModel() const {
  return *model;
}

BPMN::Node* DataProvider::findNode(const std::string& nodeId) const {
  for (auto& process : model->processes) {
    if (process->id == nodeId) {
      return process.get();
    }
    auto* node = process->find([&nodeId](BPMN::Node* n) { return n->id == nodeId; });
    if (node) {
      return node;
    }
  }
  throw std::runtime_error("DataProvider: node '" + nodeId + "' not found in model");
}

void DataProvider::evaluateGlobal(const std::string& initializationString,
                                   const LIMEX::Handle<double>& handle) {
  auto [attributeName, expressionString] = parseInitialization(initializationString);

  // Find global attribute by name
  const Attribute* attribute = nullptr;
  for (auto& [id, globalAttribute] : attributes[nullptr]) {
    if (globalAttribute->name == attributeName) {
      attribute = globalAttribute;
      break;
    }
  }
  if (!attribute) {
    throw std::runtime_error("DataProvider: unknown global attribute '" + attributeName + "'");
  }
  if (attribute->expression) {
    throw std::runtime_error("DataProvider: global attribute '" + attributeName +
                             "' is initialized by model expression and must not be provided explicitly");
  }

  // Build globals vector from current globalValueMap
  Values globals(model->attributes.size());
  for (auto& [globalAttribute, value] : globalValueMap) {
    globals[globalAttribute->index] = value;
  }

  // Compile and evaluate expression
  Expression expression(handle, expressionString, model->attributeRegistry);

  // Validate all referenced globals are already evaluated
  for (auto* referencedAttribute : expression.variables) {
    if (!globalValueMap.contains(referencedAttribute)) {
      throw std::runtime_error("DataProvider: global '" + attributeName +
                               "' references unevaluated global '" + referencedAttribute->name + "'");
    }
  }

  auto value = expression.execute(Values{}, Values{}, globals);
  if (!value.has_value()) {
    throw std::runtime_error("DataProvider: failed to evaluate global '" + attributeName + "'");
  }
  globalValueMap[attribute] = value.value();
}

std::pair<const Attribute*, std::string> DataProvider::lookupAttribute(
    const BPMN::Node* node,
    const std::string& initializationString) const {
  auto [attributeName, expressionString] = parseInitialization(initializationString);

  auto extensionElements = node->extensionElements->as<ExtensionElements>();
  if (!extensionElements->attributeRegistry.contains(attributeName)) {
    throw std::runtime_error("DataProvider: node '" + node->id +
                             "' has no attribute '" + attributeName + "'");
  }

  auto attribute = extensionElements->attributeRegistry[attributeName];
  if (attribute->expression) {
    throw std::runtime_error("DataProvider: attribute '" + attributeName +
                             "' is initialized by model expression and must not be provided explicitly");
  }

  return {attribute, expressionString};
}

std::pair<std::string, std::string> DataProvider::parseInitialization(
    const std::string& initialization) {
  auto position = initialization.find(":=");
  if (position == std::string::npos) {
    throw std::runtime_error("DataProvider: initialization must be in format 'attribute := expression', got '" + initialization + "'");
  }

  std::string attributeName = initialization.substr(0, position);
  std::string expression = initialization.substr(position + 2);

  // Trim whitespace from attribute name
  auto trimStart = attributeName.find_first_not_of(" \t");
  auto trimEnd = attributeName.find_last_not_of(" \t");
  if (trimStart == std::string::npos) {
    throw std::runtime_error("DataProvider: empty attribute name in initialization '" + initialization + "'");
  }
  attributeName = attributeName.substr(trimStart, trimEnd - trimStart + 1);

  // Trim whitespace from expression
  trimStart = expression.find_first_not_of(" \t");
  trimEnd = expression.find_last_not_of(" \t");
  if (trimStart == std::string::npos) {
    throw std::runtime_error("DataProvider: empty expression in initialization '" + initialization + "'");
  }
  expression = expression.substr(trimStart, trimEnd - trimStart + 1);

  return {attributeName, expression};
}

BPMNOS::number DataProvider::evaluateExpression(
    const std::string& expressionString,
    const LIMEX::Handle<double>& handle) const {
  Values globals(model->attributes.size());
  for (auto& [attribute, value] : globalValueMap) {
    globals[attribute->index] = value;
  }

  Expression expression(handle, expressionString, model->attributeRegistry);

  for (auto* attribute : expression.variables) {
    if (attribute->category != Attribute::Category::GLOBAL) {
      throw std::runtime_error("DataProvider: expression '" + expressionString +
                               "' references non-global attribute '" + attribute->name + "'");
    }
  }

  auto value = expression.execute(Values{}, Values{}, globals);
  if (!value.has_value()) {
    throw std::runtime_error("DataProvider: failed to evaluate expression '" + expressionString + "'");
  }
  return value.value();
}

BPMNOS::number DataProvider::evaluateExpression(
    size_t instanceId,
    const BPMN::Node* node,
    const std::string& expressionString,
    const LIMEX::Handle<double>& handle) const {

  auto extensionElements = node->extensionElements->as<ExtensionElements>();

  // Build context from parse-time evaluated values for this instance
  Values status(extensionElements->attributeRegistry.statusAttributes.size());
  Values data(extensionElements->attributeRegistry.dataAttributes.size());
  Values globals(model->attributes.size());

  // Populate globals from globalValueMap
  for (auto& [attribute, attributeValue] : globalValueMap) {
    globals[attribute->index] = attributeValue;
  }

  // Populate status/data from this instance's parse-time evaluated values
  if (parseTimeEvaluatedValues.contains(instanceId)) {
    for (auto& [attribute, attributeValue] : parseTimeEvaluatedValues.at(instanceId)) {
      if (attribute->category == Attribute::Category::STATUS) {
        status[attribute->index] = attributeValue;
      }
      else if (attribute->category == Attribute::Category::DATA) {
        data[attribute->index] = attributeValue;
      }
    }
  }

  // Compile expression using node's attributeRegistry
  Expression expression(handle, expressionString, extensionElements->attributeRegistry);

  // Validate all referenced attributes are available
  for (auto* attribute : expression.variables) {
    if (attribute->category == Attribute::Category::GLOBAL) {
      if (!globalValueMap.contains(attribute)) {
        throw std::runtime_error("DataProvider: expression '" + expressionString +
                                 "' references unevaluated global attribute '" + attribute->name + "'");
      }
    }
    else {
      // Status or data attribute - must be in this instance's parse-time evaluated values
      if (!parseTimeEvaluatedValues.contains(instanceId) ||
          !parseTimeEvaluatedValues.at(instanceId).contains(attribute)) {
        throw std::runtime_error("DataProvider: expression '" + expressionString +
                                 "' references unevaluated attribute '" + attribute->name + "'");
      }
    }
  }

  auto value = expression.execute(status, data, globals);
  if (!value.has_value()) {
    throw std::runtime_error("DataProvider: failed to evaluate expression '" + expressionString + "'");
  }
  return value.value();
}
