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
  LegacyScenario(const Model* model, BPMNOS::number inception, BPMNOS::number completion, const DataInput& attributes, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap);
  /**
   * @brief Copy constructor for LegacyScenario.
   */
  LegacyScenario(const LegacyScenario& other);

  const Model* getModel() const override;

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const override;
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getAnticipatedInstantiations(const BPMNOS::number currentTime, const BPMNOS::number assumedTime) const;

  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getAnticipatedInstances(const BPMNOS::number currentTime) const;

  std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  std::optional<BPMNOS::number> getAnticipatedValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const;
  std::optional<BPMNOS::number> getAnticipatedValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const;

  BPMNOS::Values getAnticipatedInitialStatus(const InstanceData*, const BPMNOS::number currentTime) const;
  BPMNOS::Values getAnticipatedInitialData(const InstanceData*, const BPMNOS::number currentTime) const;

  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  BPMNOS::Values getAnticipatedValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;
  BPMNOS::Values getAnticipatedData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, Data instantiation) override;
  void removeAnticipatedInstance(const BPMNOS::number instanceId);

  Data& getInstantiationData(const BPMNOS::number instanceId) override;
  Data& getAttributeData(const BPMNOS::number instanceId, const Attribute* attribute) override;
  void addAnticipation( Data& data, Disclosure anticipation );
  void setRealization( Data& data, Disclosure realization ) override;

protected:
  const DataInput& attributes; ///< Map holding all attributes in the model with keys being the process and attribute id
  std::unordered_map<size_t, InstanceData > instances; ///< Map of instances with key being the instance id.
  const Scenario::Disclosure& getLatestDisclosure(const std::vector<Scenario::Disclosure>& data, const BPMNOS::number currentTime) const;
  BPMNOS::number inception; ///< Time earliest time in execution.
  BPMNOS::number completion; ///< The latest time in execution at which an instantiation can happen.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_LegacyScenario_H
