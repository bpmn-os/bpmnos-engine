#ifndef BPMNOS_Model_DataProvider_H
#define BPMNOS_Model_DataProvider_H

#include <cassert>
#include <string>
#include <memory>
#include <unordered_map>
#include <limits>
#include <bpmn++.h>
#include <limex.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Keywords.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "Scenario.h"

namespace BPMNOS::Model {


/**
 * @brief Abstract base class representing a data provider for BPMN instance data.
 *
 * The DataProvider class is responsible for providing and managing instance data
 * for BPMN processes. 
 */
class DataProvider {
public:
  /**
   * @brief Constructor for DataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   * @param folders The folder names in which lookup tables can be found.
   */
  DataProvider(const std::string& modelFile, const std::vector<std::string>& folders);
  virtual ~DataProvider() = 0;
  const Model& getModel() const;

  virtual std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) = 0;

protected:
  std::unique_ptr<Model> model;  ///< Pointer to the BPMN model.

  DataInput attributes; ///< Map holding all attributes in the model with keys being the process (or nullptr for global attributes) and attribute id. TODO: Remove when old CSV format support is removed.

  BPMN::Node* findNode(const std::string& nodeId) const;

  /**
   * @brief Parse an initialization string in the format "attribute := expression".
   *
   * @param initialization The initialization string to parse.
   * @return A pair containing the attribute name and expression string.
   */
  static std::pair<std::string, std::string> parseInitialization(
      const std::string& initialization);

  /**
   * @brief Look up an attribute from an initialization string.
   *
   * Parses the initialization, finds the attribute in the node's attributeRegistry,
   * and validates it doesn't have a model expression.
   *
   * @param node The node whose attributeRegistry to search.
   * @param initializationString The initialization in "attribute := expression" format.
   * @return A pair containing the attribute pointer and the expression string.
   */
  std::pair<const Attribute*, std::string> lookupAttribute(
      const BPMN::Node* node,
      const std::string& initializationString) const;

  /**
   * @brief Evaluate a global attribute initialization from CSV.
   *
   * Parses the initialization string, finds the global attribute, validates it
   * doesn't have a model expression, evaluates the expression, and stores the
   * result in globalValueMap.
   *
   * @param initializationString The initialization in "attribute := expression" format.
   * @param handle The LIMEX handle to use for expression compilation.
   */
  void evaluateGlobal(const std::string& initializationString,
                      const LIMEX::Handle<double>& handle);

  /**
   * @brief Evaluate a global attribute initialization using the default handle.
   */
  virtual void evaluateGlobal(const std::string& initializationString) {
    evaluateGlobal(initializationString, model->limexHandle);
  }

  /**
   * @brief Evaluate an expression using the provided LIMEX handle.
   *
   * @param expressionString The expression to evaluate.
   * @param handle The LIMEX handle to use for evaluation.
   * @return The evaluated numeric value.
   */
  BPMNOS::number evaluateExpression(
      const std::string& expressionString,
      const LIMEX::Handle<double>& handle) const;

  /**
   * @brief Evaluate an expression using the provider's default handle.
   *
   * Uses model->limexHandle by default. Override if a different handle is needed.
   *
   * @param expressionString The expression to evaluate.
   * @return The evaluated numeric value.
   */
  virtual BPMNOS::number evaluateExpression(const std::string& expressionString) const {
    return evaluateExpression(expressionString, model->limexHandle);
  }

  /**
   * @brief Ensure a default attribute value is set for an instance.
   *
   * @tparam InstanceData Type with .process and .data members.
   * @param instance The instance data to check/update.
   * @param attributeId The attribute ID to ensure.
   * @param value Optional value to set if attribute is missing.
   */
  template<typename InstanceData>
  void ensureDefaultValue(InstanceData& instance, const std::string& attributeId,
                          std::optional<BPMNOS::number> value = std::nullopt);

  /// Map of global attributes to their evaluated values.
  std::unordered_map<const Attribute*, BPMNOS::number> globalValueMap;

  /// Earliest instantiation time across all instances.
  BPMNOS::number earliestInstantiation = std::numeric_limits<BPMNOS::number>::max();

  /// Latest instantiation time across all instances.
  BPMNOS::number latestInstantiation = std::numeric_limits<BPMNOS::number>::min();

  /// Per-instance tracking of parse-time evaluated attributes.
  /// Key: instanceId, Value: map of attribute -> evaluated value.
  /// Used to allow CSV expressions to reference previously-evaluated attributes.
  std::unordered_map<size_t, std::unordered_map<const Attribute*, BPMNOS::number>> parseTimeEvaluatedValues;

  /**
   * @brief Evaluate an expression in the context of a specific instance.
   *
   * This method allows CSV expressions to reference:
   * - Global attributes (if already evaluated)
   * - Status/data attributes from the same instance (if previously parse-time evaluated)
   *
   * @param instanceId The instance ID for context lookup.
   * @param node The node whose attributeRegistry to use for compilation.
   * @param expressionString The expression to evaluate.
   * @param type The target value type for conversion.
   * @param handle The LIMEX handle to use for compilation.
   * @return The evaluated and type-converted numeric value.
   */
  BPMNOS::number evaluateExpression(
      size_t instanceId,
      const BPMN::Node* node,
      const std::string& expressionString,
      ValueType type,
      const LIMEX::Handle<double>& handle) const;

  /**
   * @brief Evaluate an expression for an instance using the default handle.
   *
   * @param instanceId The instance ID for context lookup.
   * @param node The node whose attributeRegistry to use.
   * @param expressionString The expression to evaluate.
   * @param type The target value type for conversion.
   * @return The evaluated and type-converted numeric value.
   */
  virtual BPMNOS::number evaluateExpression(
      size_t instanceId,
      const BPMN::Node* node,
      const std::string& expressionString,
      ValueType type) const {
    return evaluateExpression(instanceId, node, expressionString, type, model->limexHandle);
  }
};

template<typename InstanceData>
void DataProvider::ensureDefaultValue(InstanceData& instance, const std::string& attributeId,
                                       std::optional<BPMNOS::number> value) {
  assert(attributes.contains(instance.process));
  auto it1 = attributes.at(instance.process).find(attributeId);
  if (it1 == attributes.at(instance.process).end()) {
    throw std::runtime_error("DataProvider: unable to find required attribute '" +
                             attributeId + "' for process '" + instance.process->id + "'");
  }
  auto attribute = it1->second;
  if (auto it2 = instance.data.find(attribute); it2 == instance.data.end()) {
    if (attribute->expression) {
      throw std::runtime_error("DataProvider: initial value of default attribute '" +
                               attribute->id + "' must not be provided by expression");
    }

    if (value.has_value()) {
      instance.data[attribute] = value.value();
    }
    else if (attributeId == BPMNOS::Keyword::Timestamp) {
      instance.data[attribute] = 0;
    }
    else {
      throw std::runtime_error("DataProvider: attribute '" + attribute->id + "' has no default value");
    }
  }
}

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_DataProvider_H
