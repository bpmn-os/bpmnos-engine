#ifndef BPMNOS_Execution_StaticDataProvider_H
#define BPMNOS_Execution_StaticDataProvider_H

#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <bpmn++.h>
#include <limex.h>
#include "DataProvider.h"
#include "Scenario.h"
#include "InstanceDataReader.h"
#include "InstanceValues.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Data provider for instance data known from the start of a run.
 *
 * The data is read from a CSV table with the columns `INSTANCE_ID`, `NODE_ID` and `INITIALIZATION`. Every
 * instance is instantiated at the first instant of a run, and its process becomes
 * ready at its instantiation time, which is the timestamp of the instance rounded up. A token arriving at an
 * activity becomes ready at once, with the status it arrived with followed by the values of the activity, and
 * a task other than a send, receive or decision task completes with the status it became busy with at the
 * timestamp of that status. An event due later is scheduled and enqueued when the @ref ClockTickEvent
 * advancing to its time is announced. A run ends when nothing is left to do, no instance is left and no event
 * is scheduled, or when nothing is left to do at the end time.
 *
 * The time at which every kind of event becomes due is given by a virtual method, which a derived data
 * provider may override.
 */
class StaticDataProvider : public DataProvider {
public:
  /**
   * @brief Scenario holding the events scheduled for a run.
   */
  class Scenario : public Execution::Scenario {
  public:
    Scenario(std::shared_ptr<const StaticDataProvider> dataProvider);

    /// Events due later, by the time they are due, each produced when it becomes due, so that a ready event is built
    /// from the data and globals of that time; a producer returns nothing if the token it concerns no longer exists
    std::multimap<BPMNOS::number, std::function<std::shared_ptr<Event>(Scenario&)>> scheduledEvents;
    BPMNOS::number time = std::numeric_limits<BPMNOS::number>::lowest(); ///< The current time of the run
  };

  /**
   * @param model The model the data provider is built on.
   * @param instanceFileOrString The name of the CSV file holding the instance data or its content.
   * @param clockTickDuration Wall-clock time between two clock ticks, zero advancing time at once and
   *        std::chrono::milliseconds::max() never.
   */
  StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, std::chrono::milliseconds clockTickDuration = std::chrono::milliseconds::zero());

  /**
   * @brief Method creating the scenario of a run. A static data provider has a single realisation.
   */
  std::unique_ptr<Execution::Scenario> createScenario(unsigned int realisation = 0) const override;

  /**
   * @brief Method setting the end time, at which a run ends once nothing is left to do.
   *
   * The end time applies to every run on a scenario of the data provider.
   */
  void setEndTime(BPMNOS::number endTime);
  BPMNOS::number getEndTime() const;

  void notice(const Observable* observable, Execution::Scenario& scenario, EventQueue& queue) const override;
  void dispatchEvent(const SystemState* systemState, Execution::Scenario& scenario, EventQueue& queue) const override;
  void advance(const SystemState* systemState, Execution::Scenario& scenario, EventQueue& queue) const override;
  BPMNOS::Data getGlobals(const Execution::Scenario& scenario) const override;
  BPMNOS::number getEarliestInstantiationTime(const Execution::Scenario& scenario) const override;

protected:
  /**
   * @brief Constructor for a derived data provider, which reads the instance data itself.
   */
  StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration);

  /**
   * @brief Method reading the instance data with the given columns, giving every row of an instance to
   * @ref readValue and evaluating every global value with the given handle.
   */
  void readInstances(const std::string& instanceFileOrString, const std::vector<std::string>& columns, const LIMEX::Handle<double>& handle);

  /**
   * @brief Method reading a row of an instance, evaluating its initialization at once with the given handle
   * and ignoring every further column.
   */
  virtual void readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle);

  /// @brief Method returning the values of the attributes of an instance, which are those read.
  virtual const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& getInstanceValues(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the objects the instance data give an instance, which are those read.
  virtual const std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>& getInstanceObjects(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the time at which an instance becomes known, which is the start of the run.
  virtual BPMNOS::number getKnownTime(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the time at which the process of an instance becomes ready, which is its
  /// instantiation time.
  virtual BPMNOS::number getProcessReadyTime(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the timestamp a token arriving at an activity becomes ready with, if a ready value gives
  /// it, which is never the case for a static data provider.
  ///
  /// The timestamp is determined when the token arrives, since the time at which the ready event becomes due
  /// depends on it. For a token awaiting its ready event in an installed system state it is determined anew and
  /// must not precede the given time, which is the time of that state; for a token arriving at the activity, the
  /// given time is the lowest number.
  virtual std::optional<BPMNOS::number> getReadyTimestamp(Scenario& scenario, const Token* token, BPMNOS::number earliest) const;

  /// @brief Method returning the time at which a token of an instance arriving at an activity becomes ready, given
  /// the timestamp a ready value gives, which is the start of the run, so that it becomes ready at once.
  virtual BPMNOS::number getActivityReadyTime(const Scenario& scenario, size_t instanceId, const BPMN::Node* activity, std::optional<BPMNOS::number> readyTimestamp) const;

  /// @brief Method applying the ready values of a token over the initial values of the scope instance of the
  /// activity, when its ready event becomes due; a static data provider gives no ready values.
  virtual void applyReadyValues(Scenario& scenario, const Token* token, InstanceValues& values) const;

  /// @brief Method returning the status a busy task completes with at its timestamp, which is the status it
  /// became busy with.
  ///
  /// The given time is that of @ref getReadyTimestamp for a token awaiting its completion event in an
  /// installed system state, and the lowest number for a task becoming busy.
  virtual BPMNOS::Status getCompletionStatus(Scenario& scenario, const Token* token, BPMNOS::number earliest) const;

  /**
   * @brief Instance data read.
   */
  struct Instance {
    size_t id;
    const BPMN::Process* process;
    std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number> values;
    BPMNOS::number instantiationTime;
    std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>> objects; ///< The objects the instance data gives
  };

  /// @brief Method returning the object attributes of the scopes of a process, or of every process and the global
  /// objects if no process is given.
  std::vector<const BPMNOS::Model::Attribute*> objectsOf(const BPMN::Process* process = nullptr) const;

  /// @brief Method returning the instance with the given identifier.
  const Instance& getInstance(size_t instanceId) const;

  /// The instances in the order of their first rows, which is the order in which instances becoming known at the
  /// same time are instantiated. The identifier of an instance is an index of the string registry, which depends on
  /// what was registered before, so an order following identifiers would make a run depend on it.
  std::vector<Instance> instances;
  std::unordered_map<size_t, size_t> instancePositions; ///< The position of each instance in @ref instances, by its identifier
  BPMNOS::Data globals; ///< The values of the global attributes at the start of a run
  BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max();

private:
  std::shared_ptr<Event> instantiate(const Scenario& scenario, const Instance& instance) const;
  void readyProcess(Scenario& scenario, EventQueue& queue, const Token* token) const;
  void readyActivity(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const;
  void completeTask(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const;
  static void schedule(Scenario& scenario, EventQueue& queue, BPMNOS::number dueTime, BPMNOS::number currentTime, std::function<std::shared_ptr<Event>(Scenario&)> produce);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StaticDataProvider_H
