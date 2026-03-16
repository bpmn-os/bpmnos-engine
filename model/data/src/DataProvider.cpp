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
