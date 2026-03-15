#ifndef BPMNOS_Model_StaticScenario_H
#define BPMNOS_Model_StaticScenario_H

#include "Scenario.h"

namespace BPMNOS::Model {

class StaticDataProvider;

/**
 * @brief A scenario implementation where all data is known from the start.
 *
 * StaticScenario is optimized for cases where all instance data and attribute
 * values are known at time 0. The currentTime parameter is ignored in query methods.
 */
class StaticScenario : public Scenario {
  friend class StaticDataProvider;

public:
  StaticScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap);

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

protected:
  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime);
  void setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);

  std::unordered_map<size_t, InstanceData> instances;
  BPMNOS::number earliestInstantiationTime;
  BPMNOS::number latestInstantiationTime;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StaticScenario_H
