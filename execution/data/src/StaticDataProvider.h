#ifndef BPMNOS_Execution_StaticDataProvider_H
#define BPMNOS_Execution_StaticDataProvider_H

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

    std::multimap<BPMNOS::number, std::shared_ptr<Event>> scheduledEvents; ///< Events due later, by the time they are due
  };

  /**
   * @param model The model the data provider is built on.
   * @param instanceFileOrString The name of the CSV file holding the instance data or its content.
   * @param clockTickDuration Milliseconds of wall-clock time between two clock ticks, zero meaning none.
   */
  StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int clockTickDuration = 0);

  /**
   * @brief Method creating the scenario of a run. A static data provider has a single realisation.
   */
  std::unique_ptr<Scenario> createScenario() const;

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
  BPMNOS::Values getGlobals(const Execution::Scenario& scenario) const override;
  BPMNOS::number getEarliestInstantiationTime(const Execution::Scenario& scenario) const override;

protected:
  /**
   * @brief Constructor for a derived data provider, which reads the instance data itself.
   */
  StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, unsigned int clockTickDuration);

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

  /// @brief Method returning the time at which an instance becomes known, which is the start of the run.
  virtual BPMNOS::number getKnownTime(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the time at which the process of an instance becomes ready, which is its
  /// instantiation time.
  virtual BPMNOS::number getProcessReadyTime(const Scenario& scenario, size_t instanceId) const;

  /// @brief Method returning the status a token arriving at an activity becomes ready with, which is the
  /// status it arrived with followed by the values of the activity.
  virtual BPMNOS::Values getActivityReadyStatus(Scenario& scenario, const Token* token) const;

  /// @brief Method returning the time at which a token of an instance arriving at an activity becomes ready
  /// with the given status, which is the start of the run, so that it becomes ready at once.
  virtual BPMNOS::number getActivityReadyTime(const Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Values& readyStatus) const;

  /// @brief Method returning the status a busy task completes with at its timestamp, which is the status it
  /// became busy with.
  virtual BPMNOS::Values getCompletionStatus(Scenario& scenario, const Token* token) const;

  /// @brief Method returning the value of an attribute of an instance, computed from the values of the
  /// instance if the model assigns it, and std::nullopt if it is not known.
  std::optional<BPMNOS::number> getValue(const Scenario& scenario, size_t instanceId, const BPMNOS::Model::Attribute* attribute) const;

  /// @brief Method returning the values of the status attributes a node declares.
  BPMNOS::Values getStatus(const Scenario& scenario, size_t instanceId, const BPMN::Node* node) const;

  /// @brief Method returning the values of the data attributes a node declares.
  BPMNOS::Values getData(const Scenario& scenario, size_t instanceId, const BPMN::Node* node) const;

  /**
   * @brief Instance data read.
   */
  struct Instance {
    const BPMN::Process* process;
    std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number> values;
    BPMNOS::number instantiationTime;
  };

  const std::shared_ptr<const BPMNOS::Model::Model> sharedModel; ///< The model, whose ownership the data provider shares
  std::unordered_map<size_t, Instance> instances; ///< The instances by their identifier
  BPMNOS::Values globals; ///< The values of the global attributes at the start of a run
  BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max();

private:
  void readyProcess(Scenario& scenario, EventQueue& queue, const Token* token) const;
  void readyActivity(Scenario& scenario, EventQueue& queue, const Token* token) const;
  void completeTask(Scenario& scenario, EventQueue& queue, const Token* token) const;
  static void schedule(Scenario& scenario, EventQueue& queue, BPMNOS::number dueTime, BPMNOS::number currentTime, std::shared_ptr<Event> event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StaticDataProvider_H
