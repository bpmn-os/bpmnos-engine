#ifndef BPMNOS_Model_DynamicScenario_H
#define BPMNOS_Model_DynamicScenario_H

#include <bpmn++.h>
#include "Scenario.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <memory>
#include <set>

namespace BPMNOS::Model {

/**
 * @brief Structure representing a pending disclosure.
 *
 * Stores information needed to evaluate an initialization expression
 * when disclosure time is reached.
 */
struct PendingDisclosure {
  const Attribute* attribute;              ///< The attribute to initialize
  BPMNOS::number disclosureTime;           ///< Time when this attribute is disclosed
  std::unique_ptr<Expression> expression;  ///< Compiled expression to evaluate at disclosure time
};

/**
 * @brief A scenario implementation where data may be revealed over time.
 *
 * DynamicScenario supports cases where instance data and attribute
 * values become known at different points in time.
 */
class DynamicDataProvider;

class DynamicScenario : public Scenario {
  friend class DynamicDataProvider;

public:
  DynamicScenario(const Model* model, BPMNOS::number earliestInstantiationTime, BPMNOS::number latestInstantiationTime, const std::unordered_map< const Attribute*, BPMNOS::number >& globalValueMap);

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector< const InstanceData* > getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector< const InstanceData* > getKnownInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  void revealData(BPMNOS::number currentTime) const;

protected:
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime);
  void setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);
  void setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number disclosureTime);
  void addPendingDisclosure(const BPMNOS::number instanceId, PendingDisclosure&& pending);

  mutable std::unordered_map<size_t, InstanceData> instances;
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosure; ///< Instance ID -> Node -> time when node's data is disclosed
  mutable std::unordered_map<size_t, std::vector<PendingDisclosure>> pendingDisclosures; ///< Instance ID -> pending disclosures
  mutable std::set<std::pair<size_t, const Attribute*>> disclosedAttributes; ///< Track which attributes have been disclosed
  BPMNOS::number earliestInstantiationTime;
  BPMNOS::number latestInstantiationTime;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_DynamicScenario_H
