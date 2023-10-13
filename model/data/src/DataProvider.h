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

  virtual std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) = 0;

protected:
  std::unique_ptr<Model> model;  ///< Pointer to the BPMN model.

  DataInput attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
};

} // namespace BPMNOS::Model

#endif // BPMNOS_DataProvider_H
