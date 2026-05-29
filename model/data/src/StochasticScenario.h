#ifndef BPMNOS_Model_StochasticScenario_H
#define BPMNOS_Model_StochasticScenario_H

#include <bpmn++.h>
#include "Scenario.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/RandomDistributionFactory.h"
#include <memory>
#include <random>
#include <set>
#include <map>

namespace BPMNOS::Model {

/**
 * @brief Structure representing a pending disclosure.
 *
 * Stores a pre-computed value to be revealed at disclosure time.
 * The value is computed at parse time using only global attributes.
 */
struct StochasticPendingDisclosure {
  const Attribute* attribute;     ///< The attribute to initialize
  BPMNOS::number disclosureTime;  ///< Time when this attribute is disclosed
  BPMNOS::number value;           ///< Pre-computed value to reveal at disclosure time
};

/**
 * @brief Structure representing a deferred disclosure.
 *
 * Stores shared expressions to be evaluated per-scenario for independent sampling.
 * Expression objects are immutable; different results come from different RNG contexts.
 */
struct DeferredDisclosure {
  const Attribute* attribute;                    ///< The attribute to initialize
  const BPMN::Node* node;                        ///< The node scope
  std::shared_ptr<Expression> initializationExpression;  ///< Expression to compute value
  std::shared_ptr<Expression> disclosureExpression;      ///< Expression to compute disclosure time
};

/**
 * @brief A scenario implementation supporting stochastic behavior.
 *
 * StochasticScenario supports:
 * - Random functions in INITIALIZATION, DISCLOSURE, and COMPLETION expressions
 * - Per (instance, node) RNG for reproducibility
 * - Downward compatible with static (3-column) and dynamic (4-column) CSV formats
 */
class StochasticDataProvider;

class StochasticScenario : public Scenario {
  friend class StochasticDataProvider;

public:
  StochasticScenario(
    const Model* model,
    const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap,
    unsigned int seed = 0
  );

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>> getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector<const InstanceData*> getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector<const InstanceData*> getInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getStatus(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getActivityReadyStatus(BPMNOS::number instanceId, const BPMN::Node* activity, BPMNOS::number currentTime) const override;

  /**
   * @brief Make scenario aware of a token arriving at an activity.
   *
   * Evaluates READY expressions using the parent scope's context and stores
   * the computed values.
   */
  void noticeReadyPending( BPMNOS::number instanceId, const BPMN::Node* node, const Values& status, const SharedValues& data, const Values& globals ) const override;

  /**
   * @brief Make scenario aware of a task entering BUSY state.
   *
   * Evaluates COMPLETION expressions immediately and stores the final result.
   * Uses per-(instance, node) RNG for reproducibility.
   */
  void noticeCompletionPending(BPMNOS::number instanceId, const BPMN::Node* task, const Values& status, const SharedValues& data,     const Values& globals) const override;

  void revealData(BPMNOS::number currentTime) const;

private:
  void initializeActivityData(BPMNOS::number instanceId, const BPMN::Node* node, const Values& status, const SharedValues& data, const Values& globals) const;

  void setTaskCompletionStatus(BPMNOS::number instanceId, const BPMN::Node* task, const Values& status, const SharedValues& data, const Values& globals) const;

  /// Compute earliestInstantiationTime and latestInstantiationTime after deferred disclosures are evaluated
  void computeInstantiationBounds();

protected:
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId);
  void setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);
  void setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number disclosureTime);
  void addPendingDisclosure(const BPMNOS::number instanceId, StochasticPendingDisclosure&& pending);
  void addCompletionExpression(const BPMNOS::number instanceId, const BPMN::Node* task, std::shared_ptr<Expression> expression);
  void addReadyExpression(const BPMNOS::number instanceId, const BPMN::Node* node, std::shared_ptr<Expression> expression);
  void addDeferredDisclosure(const BPMNOS::number instanceId, DeferredDisclosure&& deferred);

  /// Evaluate all deferred disclosures using scenario-specific RNG
  void evaluateDeferredDisclosures();

  /// Get or create RNG for (instance, node) pair
  std::mt19937& getRng(size_t instanceId, const BPMN::Node* node) const;

  mutable std::unordered_map<size_t, InstanceData> instances;
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosure; ///< Instance ID -> Node -> disclosure time
  mutable std::unordered_map<size_t, std::vector<StochasticPendingDisclosure>> pendingDisclosures; ///< Instance ID -> pending disclosures
  mutable std::set<std::pair<size_t, const Attribute*>> disclosedAttributes; ///< Track which attributes have been disclosed

  /// Ready expressions per (instance, node) - shared across scenario copies
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<std::shared_ptr<Expression>>>> readyExpressions;

  /// Completion expressions per (instance, node) - shared across scenario copies
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<std::shared_ptr<Expression>>>> completionExpressions;

  /// Deferred disclosures per instance (evaluated per-scenario)
  std::unordered_map<size_t, std::vector<DeferredDisclosure>> deferredDisclosures;

  /// Per (instance, node) RNG for reproducibility
  mutable std::map<std::pair<size_t, const BPMN::Node*>, std::mt19937> rngs;

  unsigned int scenarioSeed;
  BPMNOS::number earliestInstantiationTime;
  BPMNOS::number latestInstantiationTime;

  /// RandomDistributionFactory for expression evaluation (set by provider)
  mutable RandomDistributionFactory* randomFactory = nullptr;

  /// Stochastic LIMEX handle for expression evaluation (set by provider)
  const LIMEX::Handle<double>* stochasticHandle = nullptr;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StochasticScenario_H
