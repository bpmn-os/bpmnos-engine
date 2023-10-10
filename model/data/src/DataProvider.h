#ifndef BPMNOS_DataProvider_H
#define BPMNOS_DataProvider_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/Model.h"
#include "model/parser/src/extensionElements/Attribute.h"
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
   */
  DataProvider(const std::string& modelFile);
  virtual ~DataProvider() = 0;
  const Model& getModel() const;

  void createScenario(BPMNOS::number time, unsigned int scenarioId = 0);

  /**
   * @brief Method returning a vector of all instances that are known until the given time.
   */
  std::vector< const Scenario::InstanceData* > getKnownInstances(BPMNOS::number time, unsigned int scenarioId = 0) const;

  /**
   * @brief Method adding all known attribute values to the status and returning true if and only if all new attribute values are known until the given time.
   *
   * If at least one attribute value is not yet known, the method doesn't change the status.
   */
  bool getKnownValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time, unsigned int scenarioId = 0) const;

  /**
   * @brief Method returning a vector of all instances that have been disclosed until the given time.
   */
  std::vector< const Scenario::InstanceData* > getAssumedInstances(BPMNOS::number time, unsigned int scenarioId = 0) const;

  /**
   * @brief Method adding all disclosed attribute values to the status.
   */
  void getAssumedValues(const BPMN::FlowNode* node, Values& status, BPMNOS::number time, unsigned int scenarioId = 0) const;

protected:
  const std::unique_ptr<Model> model;  ///< Pointer to the BPMN model.

  /**
   * @brief Vector of scenarios holding the data that could be realized.
   *
   * The first element of this vector is reserved for the data that will actually be realized.
   */
  std::vector< std::unique_ptr<Scenario> > scenarios;
  DataInput attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
};

} // namespace BPMNOS::Model

#endif // BPMNOS_DataProvider_H
