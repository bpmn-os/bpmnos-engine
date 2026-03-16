#ifndef BPMNOS_Model_Scenario_H
#define BPMNOS_Model_Scenario_H

#include <string>
#include <memory>
#include <map>
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

  /**
   * @brief Store the completion status when a task enters BUSY state.
   *
   * @param instanceId The instance identifier.
   * @param task The task node entering BUSY state.
   * @param status The predicted completion status values.
   */
  virtual void setTaskCompletionStatus(
    const BPMNOS::number instanceId,
    const BPMN::Node* task,
    BPMNOS::Values status
  ) const {
    taskCompletionStatus[{(size_t)instanceId, task}] = std::move(status);
  }

  /**
   * @brief Get the completion status for a task.
   *
   * For deterministic scenarios, returns the value stored by setTaskCompletionStatus().
   * For stochastic scenarios, evaluates COMPLETION expressions and returns updated status.
   *
   * @param instanceId The instance identifier.
   * @param task The task node that is completing.
   * @return Completion status values.
   */
  virtual BPMNOS::Values getTaskCompletionStatus(
    const BPMNOS::number instanceId,
    const BPMN::Node* task
  ) const {
    return taskCompletionStatus.at({(size_t)instanceId, task});
  }

  /**
   * @brief Initialize arrival data when a token arrives at an activity.
   *
   * Evaluates ARRIVAL expressions using the parent scope's context.
   * Default implementation does nothing - only StochasticScenario evaluates ARRIVAL.
   *
   * @param instanceId The instance identifier.
   * @param node The activity node being entered.
   * @param status Parent scope's status attributes.
   * @param data Parent scope's data attributes.
   * @param globals Global attributes.
   */
  virtual void initializeArrivalData(
    [[maybe_unused]] BPMNOS::number instanceId,
    [[maybe_unused]] const BPMN::Node* node,
    [[maybe_unused]] const Values& status,
    [[maybe_unused]] const Values& data,
    [[maybe_unused]] const Values& globals
  ) const {}

  const Model* model;  ///< Pointer to the BPMN model.
  BPMNOS::Values globals;

  /// Stored completion status per (instanceId, task)
  mutable std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values> taskCompletionStatus;

protected:
  /// Constructor that initializes model and globals from CSV-provided values.
  Scenario(const Model* model, const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap);

  /// Evaluate global attributes (CSV-provided + model expressions).
  Values evaluateGlobals(const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap);

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
