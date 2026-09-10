#ifndef BPMNOS_Model_ObservedScenario_H
#define BPMNOS_Model_ObservedScenario_H

#include "Scenario.h"

namespace BPMNOS::Model {

/**
 * @brief A scenario whose data is observed while the run proceeds.
 *
 * Nothing is known in advance. The scenario holds a log of what has been reported to it so far, and
 * answers every query from that log, reporting `std::nullopt` for anything not yet observed, upon which
 * the engine waits and asks again at the next clock tick.
 *
 * Whatever observes the world reports through @ref observeInstantiation, @ref observeValue,
 * @ref observeReadyStatus and @ref observeCompletionStatus, and owns time, issuing the clock ticks itself.
 * An observed world admits no duplicate of itself, so @ref clone throws.
 */
class ObservedScenario : public Scenario {
public:
  /**
   * @brief Constructor for ObservedScenario.
   *
   * @param model The model to be executed.
   * @param globalValueMap The observed initial values of global attributes. Globals change thereafter
   *        through the engine and are never re-observed.
   */
  ObservedScenario(const Model* model, const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap);

  /// @name Observation
  /// Report what has been observed. An attribute reported with `std::nullopt` is observed to be undefined,
  /// which is distinct from not having been observed at all. A reported status is returned once the clock
  /// reaches the timestamp it carries, so it may be reported before it is due.
  ///@{

  /// @brief Report that an instance of the given process was created at the given time.
  void observeInstantiation(const BPMN::Process* process, BPMNOS::number instanceId, BPMNOS::number instantiationTime);

  /// @brief Report the value of an attribute of an instance.
  void observeValue(BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);

  /// @brief Report the status an activity is ready with.
  ///
  /// @param instanceId The instance identifier the activity belongs to.
  /// @param activity The activity that became ready.
  /// @param status The complete status of the activity, being the values of every attribute declared from
  ///        the process down to the activity itself, in that order.
  void observeReadyStatus(BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::Values status);

  /// @brief Report the status a task completes with.
  ///
  /// @param instanceId The instance identifier the task belongs to.
  /// @param task The task that completed.
  /// @param status The complete status of the task, laid out as for @ref observeReadyStatus.
  void observeCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, BPMNOS::Values status);

  ///@}

  /// @brief Returns the largest representable time, no instantiation being known in advance.
  BPMNOS::number getEarliestInstantiationTime() const override;

  /// @brief Returns false, an observed world never being exhausted; a run ends when it is told to.
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getValue(const InstanceData* instance, const Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getValue(const BPMNOS::number instanceId, const Attribute* attribute, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  /// @brief Returns the status reported for an activity once the clock has reached its timestamp, and
  /// std::nullopt while none has been reported or its timestamp lies ahead.
  std::optional<BPMNOS::Values> getActivityReadyStatus(BPMNOS::number rootId, BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::number currentTime) const override;

  /// @brief Returns the status reported for a task once the clock has reached its timestamp, and
  /// std::nullopt while none has been reported or its timestamp lies ahead.
  std::optional<BPMNOS::Values> getTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, BPMNOS::number currentTime) const override;

  /// @brief Discards the status reported for the activity, which has been used, and then does as the base.
  void noticeReady(BPMNOS::number instanceId, const BPMN::Node* node) const override;

  /// @brief Discards the status reported for the task, which has been used, and then does as the base.
  void noticeCompletion(BPMNOS::number instanceId, const BPMN::Node* task) const override;

  /// @brief Throws, a world admitting no duplicate of itself.
  std::unique_ptr<Scenario> clone(BPMNOS::number spawnTime, size_t index) const override;

protected:
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

private:
  /// @brief Collect the values of the given attributes of an instance, or std::nullopt if any of them has
  /// not been observed. Reporting an incomplete set would let the engine proceed on unknown values.
  std::optional<BPMNOS::Values> getObservedValues(const InstanceData& instance, const std::vector< std::unique_ptr<Attribute> >& attributes, const BPMNOS::number currentTime) const;

  /// @brief Returns a reported status once the clock has reached the timestamp it carries.
  std::optional<BPMNOS::Values> getReportedStatus(const std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values>& reported, BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number currentTime) const;

  std::unordered_map<size_t, InstanceData> instances; ///< Log of the instances observed so far.
  mutable std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values> observedReadyStatus; ///< Statuses activities were reported ready with.
  mutable std::map<std::pair<size_t, const BPMN::Node*>, BPMNOS::Values> observedCompletionStatus; ///< Statuses tasks were reported to complete with.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ObservedScenario_H
