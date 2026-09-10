#ifndef BPMNOS_Model_Scenario_H
#define BPMNOS_Model_Scenario_H

#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Value.h"
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
   * @brief Method returning a vector of all instances that are to be instantiated at the given time.
   */
  virtual std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created until the given time.
   */
  virtual std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a vector of all instances that have been created or are known until the given time.
   */
  virtual std::vector< const InstanceData* > getInstances(const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning a value of an attribute.
   *
   * If the attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::number> getValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning values for all new status attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Method returning values for all new data attributes.
   *
   * If at least one attribute value is not yet known, the method returns std::nullopt.
   */
  virtual std::optional<BPMNOS::Values> getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const = 0;

  /**
   * @brief Make scenario aware that the clock is advancing to the given time.
   *
   * The engine announces a clock tick before it processes it, so at the moment of the call the system
   * state still holds the preceding time and @p time is the time being advanced to. A scenario that owns
   * time, such as one observing real life or a world model, does not implement this method.
   *
   * @param time The time the clock is advancing to.
   */
  virtual void noticeClockTick([[maybe_unused]] BPMNOS::number time) const {}

  /**
   * @brief Make scenario aware of a an activity for which the ready status must be determined.
   *
   * Records the status the activity was reached with in @ref activityArrivalStatus, from which
   * @ref getActivityReadyStatus determines the ready status.
   *
   * @param instanceId The instance identifier.
   * @param node The activity node being entered.
   * @param status Parent scope's status attributes.
   * @param data Parent scope's data attributes.
   * @param globals Global attributes.
   */
  virtual void noticeReadyPending(BPMNOS::number instanceId, const BPMN::Node* node, const Values& status, const SharedValues& data, const Values& globals) const;

  /**
   * @brief Make scenario aware of a task for which the completion status must be determined.
   *
   * @param rootId The root instance identifier.
   * @param task The task node entering BUSY state.
   * @param status The current status values.
   * @param data The current data values.
   * @param globals Global attributes.
   */
  virtual void noticeCompletionPending(BPMNOS::number rootId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const;

  /**
   * @brief Make scenario aware that an activity has become ready.
   *
   * The arrival status recorded by @ref noticeReadyPending has been used and is discarded, so that
   * @ref activityArrivalStatus holds exactly the arrivals still awaiting a ready status.
   *
   * @param instanceId The instance identifier, disambiguated for event-subprocess and multi-instance
   *        executions, as @ref noticeReadyPending derives it from the data.
   * @param node The activity node that became ready.
   */
  virtual void noticeReady(BPMNOS::number instanceId, const BPMN::Node* node) const;

  /**
   * @brief Make scenario aware that a task has completed.
   *
   * The status recorded by @ref noticeCompletionPending has been used and is discarded, so that
   * @ref taskCompletionStatus holds exactly the tasks still awaiting a completion status.
   *
   * @param instanceId The instance identifier, disambiguated as for @ref noticeReady.
   * @param task The task node that completed.
   */
  virtual void noticeCompletion(BPMNOS::number instanceId, const BPMN::Node* task) const;

  /// @brief Returns the entry held in @ref activityArrivalStatus, or nullptr if none is held.
  const BPMNOS::Values* getActivityArrivalStatus(BPMNOS::number instanceId, const BPMN::Node* node) const;

  /// @brief Returns the entry held in @ref taskCompletionStatus, or nullptr if none is held.
  const BPMNOS::Values* getTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task) const;

  /// @brief Get the completion status for a SendTask, ReceiveTask, and DecisionTask.
  BPMNOS::Values getTaskCompletionStatus(BPMNOS::number rootId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const;

  /**
   * @brief Get the completion status for a task.
   *
   * Checks if currentTime >= completion timestamp. If yes, returns the
   * stored completion status. If no, returns std::nullopt.
   *
   * @param instanceId The instance identifier.
   * @param task The task node that is completing.
   * @param currentTime The current time for completion check.
   * @return Completion status values if completed, std::nullopt otherwise.
   */
  virtual std::optional<BPMNOS::Values> getTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, BPMNOS::number currentTime) const;

  /**
   * @brief Get the ready status for an activity.
   *
   * Checks if all node's own attributes are disclosed at currentTime.
   * If yes, combines stored parent status (from noticeActivityArrival) with
   * node's own attributes to return full status.
   * If no, returns std::nullopt.
   *
   * @param rootId The root instance identifier.
   * @param instanceId The instance identifier.
   * @param activity The activity node.
   * @param currentTime The current time for disclosure check.
   * @return Full status values if ready, std::nullopt otherwise.
   */
  virtual std::optional<BPMNOS::Values> getActivityReadyStatus(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::number currentTime) const = 0;

  /// @brief Returns an independent copy of this scenario agreeing with it before @p spawnTime, being the
  /// @p index -th such copy.
  ///
  /// The copy carries its own per-run memoization maps, so that sub-engines exploring alternative
  /// continuations neither race on them nor overwrite one another's entries. A scenario with a single
  /// realization ignores both arguments and returns an identical copy, there being nothing to resample.
  /// A scenario with many realizations returns the @p index -th of them, agreeing with this one before
  /// @p spawnTime, so that the same index yields the same realization on every call and copies taken at
  /// different indices are independent. The base throws; a scenario that cannot be copied at all, such as
  /// one observing a world that cannot be duplicated, leaves it to do so.
  ///
  /// @param spawnTime First time point at which a copy may differ from this scenario.
  /// @param index Selects which realization is returned; ignored where there is only one.
  virtual std::unique_ptr<Scenario> clone(BPMNOS::number spawnTime, size_t index) const;

  const Model* model;  ///< Pointer to the BPMN model.
  BPMNOS::Values globals;

protected:
  /// Constructor that initializes model and globals from CSV-provided values.
  Scenario(const Model* model, const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap);

  /// Protected copy constructor for derived class scenario copying.
  Scenario(const Scenario* original);

  /// Evaluate global attributes (CSV-provided + model expressions).
  Values evaluateGlobals(const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap);

  /// @brief Evaluate an attribute that the model declares by assignment expression.
  ///
  /// Such an attribute carries its own initialization in the model, so its value is computed from other
  /// attributes of the same instance rather than supplied with the instance data, and the computation is
  /// therefore the same for every scenario. Inputs are fetched through @ref getValue, so each scenario
  /// supplies them from its own storage.
  ///
  /// @return std::nullopt if an input is mutable, and so not knowable before the run, or not yet known.
  /// @pre The attribute has an expression of type Expression::Type::ASSIGN.
  std::optional<BPMNOS::number> getAssignedValue(const InstanceData* instance, const Attribute* attribute, BPMNOS::number currentTime) const;

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


  /// Stored completion status per (instanceId, task)
  mutable std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values> taskCompletionStatus;

  /// Stored arrival status per (instanceId, activity) from noticeActivityArrival
  mutable std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values> activityArrivalStatus;

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Scenario_H
