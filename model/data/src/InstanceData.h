#ifndef BPMNOS_InstanceData_H
#define BPMNOS_InstanceData_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/parser/src/extensionElements/Status.h"

namespace BPMNOS::Model {

/**
 * @brief Abstract base class representing instance data associated with a BPMN process.
 *
 * This class stores and manages the instance data for a specific BPMN process.
 * It provides access to instance data in the numeric type provided as template parameter.
 * All strings are represented by an integer and all decimals are rounded to the nearest
 * double value.
 */
class InstanceData {
public:
  /**
   * @brief Constructor for InstanceData.
   *
   * @param process The BPMN process associated with the instance data.
   * @param id The unique identifier of the instance.
   */
  InstanceData(const BPMN::Process* process, const std::string& id);

  const BPMN::Process* process;  ///< Reference to the associated BPMN process.
  const std::string id;          ///< Unique identifier of the instance.

  /**
   * @brief Method returning the known attribute value and std::nullopt if the value is not yet known with certainty or if it is known to be undefined.
   */
  std::optional<BPMNOS::number> getActualValue(const Attribute* attribute) const;

  /**
   * @brief Virtual method returning a prediction of the attribute value and std::nullopt if no prediction can be made or the value is predicted to be undefined.
   */
  virtual std::optional<BPMNOS::number> getPredictedValue(const Attribute* attribute) const = 0;

  /**
   * @brief Virtual method returning an assumption on the attribute value and std::nullopt if no assumption can be made or the value is assumed to be undefined.
   */
  virtual std::optional<BPMNOS::number> getAssumedValue(const Attribute* attribute) const = 0;

protected:
  /**
   * @brief Map holding all the actual attribute values or std::nullopt if the respective value is not yet known with certainty or if it is known to be undefined.
   */
  std::unordered_map< const Attribute*, std::optional<BPMNOS::number> > actualValues;

  /**
   * @brief Map holding all the default attribute values provided in the model.
   */
  std::unordered_map< const Attribute*, std::optional<BPMNOS::number> > defaultValues;

  /**
   * @brief Map providing access to all attributes by their id.
   */
  std::unordered_map< std::string, const Attribute* > attributes;
};

  
} // namespace BPMNOS::Model

#endif // BPMNOS_InstanceData_H
