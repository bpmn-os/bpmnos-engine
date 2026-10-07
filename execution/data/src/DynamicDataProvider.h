#ifndef BPMNOS_Execution_DynamicDataProvider_H
#define BPMNOS_Execution_DynamicDataProvider_H

#include <memory>
#include <string>
#include <unordered_map>
#include "StaticDataProvider.h"

namespace BPMNOS::Execution {

/**
 * @brief Data provider for instance data disclosed during a run.
 *
 * The data is read from a CSV table with the columns `INSTANCE_ID`, `NODE_ID`, `INITIALIZATION` and
 * `DISCLOSURE`, of which the table may omit the last. A disclosure, which requires an initialization in the
 * same row, is an expression evaluated after the initialization of its row, rounded up to an integral time,
 * and defaults to zero. The disclosure time of a node is the latest disclosure of its rows and of the scope
 * containing it, and a node whose scope has no disclosure time is rejected. An instance becomes known at the
 * disclosure time of its process, its process becomes ready at its instantiation time but not before, and a
 * token arriving at an activity becomes ready at the disclosure time of the activity but not before. In every
 * other respect a run proceeds as on the static data provider.
 */
class DynamicDataProvider : public StaticDataProvider {
public:
  /**
   * @param model The model the data provider is built on.
   * @param instanceFileOrString The name of the CSV file holding the instance data or its content.
   * @param clockTickDuration Wall-clock time between two clock ticks, zero advancing time at once and
   *        std::chrono::milliseconds::max() never.
   */
  DynamicDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, std::chrono::milliseconds clockTickDuration = std::chrono::milliseconds::zero());

protected:
  /**
   * @brief Constructor for a derived data provider, which reads the instance data itself.
   */
  DynamicDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration);

  void readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle) override;
  BPMNOS::number getKnownTime(const Scenario& scenario, size_t instanceId) const override;
  BPMNOS::number getProcessReadyTime(const Scenario& scenario, size_t instanceId) const override;
  BPMNOS::number getActivityReadyTime(const Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Values& readyStatus) const override;

  /// @brief Method returning the disclosure time of every node of an instance with a row, which are those read.
  virtual const std::unordered_map<const BPMN::Node*, BPMNOS::number>& getDisclosureTimes(const Scenario& scenario, size_t instanceId) const;

  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosureTimes; ///< The disclosure time of every node with a row, for each instance
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DynamicDataProvider_H
