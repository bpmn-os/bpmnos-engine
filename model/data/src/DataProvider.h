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
