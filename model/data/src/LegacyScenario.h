#ifndef BPMNOS_Model_LegacyScenario_H
#define BPMNOS_Model_LegacyScenario_H

#include "Scenario.h"

namespace BPMNOS::Model {

/**
 * @brief The LegacyScenario class holds data for all BPMN instances.
 *
 * This is the original Scenario implementation, now inheriting from the abstract Scenario base class.
 */
class LegacyScenario : public Scenario {
public:
  /**
   * @brief Constructor for LegacyScenario.
   */
  LegacyScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const DataInput& attributes, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap);
  /**
   * @brief Copy constructor for LegacyScenario.
   */
  LegacyScenario(const LegacyScenario& other);

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime);
  void setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);

protected:
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  const DataInput& attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
  std::unordered_map<size_t, InstanceData> instances; ///< Map of instances with key being the instance id.
  BPMNOS::number earliestInstantiationTime; ///< Earliest time in execution.
  BPMNOS::number latestInstantiationTime; ///< The latest time in execution at which an instantiation can happen.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_LegacyScenario_H
