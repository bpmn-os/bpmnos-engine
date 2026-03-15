#ifndef BPMNOS_Model_Scenario_H
#define BPMNOS_Model_Scenario_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Model {

typedef std::unordered_map< const BPMN::Process*, std::unordered_map< std::string, const Attribute* > > DataInput;

/**
 * @brief Abstract base class for scenarios holding data for all BPMN instances.
 */
class Scenario {
public:
  struct InstanceData {
    const BPMN::Process* process;
    size_t id; ///< Instance identifier.
    BPMNOS::number instantiationTime; ///< Time at which the instance is instantiated.
    std::unordered_map< const Attribute*, std::optional<BPMNOS::number> > values; ///< Attribute values.
  };

  virtual ~Scenario() = default;

  static constexpr char delimiters[] = {'^','#'}; ///< Delimiters used for disambiguation of identifiers of non-interrupting event subprocesses and multi-instance activities

  /**
   * @brief Method returning the model.
   */
  const Model* getModel() const { return model; }

  /**
   * @brief Method returning the time of the earliest instantiation.
   */
  virtual BPMNOS::number getEarliestInstantiationTime() const = 0;

  /**
   * @brief Method returning true if the currentTime exceeds the completion time.
   */
  virtual bool isCompleted(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that are known to be instantiated at the given time.
   */
  virtual std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created until the given time.
   */
  virtual std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created or are known for sure until the given time.
   */
  virtual std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a known value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a known value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning all known values of new attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  BPMNOS::Values globals;
  const Model* model;  ///< Pointer to the BPMN model.

protected:
  /**
   * @brief Method returning the initial status attributes for process instantiation.
   *
   * Used internally by getCurrentInstantiations to get process-level status attributes.
   */
  virtual Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const = 0;

  /**
   * @brief Method returning the initial data attributes for process instantiation.
   *
   * Used internally by getCurrentInstantiations to get process-level data attributes.
   */
  virtual Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const = 0;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Scenario_H
