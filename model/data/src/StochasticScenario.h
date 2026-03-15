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
 * @brief Structure representing an arrival expression.
 *
 * Stores the compiled expression to evaluate when a token arrives at an activity.
 */
struct ArrivalExpression {
  const Attribute* attribute;              ///< The attribute to initialize
  std::unique_ptr<Expression> expression;  ///< Compiled expression to evaluate at arrival time
};

/**
 * @brief Structure representing a completion expression.
 *
 * Stores the compiled expression to evaluate when a task completes.
 */
struct CompletionExpression {
  const Attribute* attribute;              ///< The attribute to update
  std::unique_ptr<Expression> expression;  ///< Compiled expression to evaluate at completion time
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
    BPMNOS::number earliestInstantiationTime,
    BPMNOS::number latestInstantiationTime,
    const std::unordered_map<const Attribute*, BPMNOS::number>& globalValueMap,
    unsigned int seed = 0
  );

  BPMNOS::number getEarliestInstantiationTime() const override;
  bool isCompleted(const BPMNOS::number currentTime) const override;

  std::vector<std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values>> getCurrentInstantiations(const BPMNOS::number currentTime) const override;

  std::vector<const InstanceData*> getCreatedInstances(const BPMNOS::number currentTime) const override;
  std::vector<const InstanceData*> getKnownInstances(const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::number> getKnownValue(const Scenario::InstanceData* instance, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::number> getKnownValue(const BPMNOS::number instanceId, const BPMNOS::Model::Attribute* attribute, const BPMNOS::number currentTime) const override;

  std::optional<BPMNOS::Values> getKnownValues(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;
  std::optional<BPMNOS::Values> getKnownData(const BPMNOS::number instanceId, const BPMN::Node* node, const BPMNOS::number currentTime) const override;

  /**
   * @brief Store the completion status when a task enters BUSY state.
   *
   * Evaluates COMPLETION expressions immediately and stores the final result.
   * Uses per-(instance, node) RNG for reproducibility.
   */
  void setTaskCompletionStatus(
    const BPMNOS::number instanceId,
    const BPMN::Node* task,
    BPMNOS::Values status
  ) const override;

  void revealData(BPMNOS::number currentTime) const;

  /**
   * @brief Initialize arrival data when a token arrives at an activity.
   *
   * Evaluates ARRIVAL expressions using the parent scope's context and stores
   * the computed values. Called by ScenarioUpdater when token enters ARRIVED
   * or CREATED state at an Activity.
   */
  void initializeArrivalData(
    BPMNOS::number instanceId,
    const BPMN::Node* node,
    const Values& status,
    const Values& data,
    const Values& globals
  ) const override;

protected:
  Values getKnownInitialStatus(const InstanceData*, const BPMNOS::number time) const override;
  Values getKnownInitialData(const InstanceData*, const BPMNOS::number time) const override;

  void addInstance(const BPMN::Process* process, const BPMNOS::number instanceId, BPMNOS::number instantiationTime);
  void setValue(const BPMNOS::number instanceId, const Attribute* attribute, std::optional<BPMNOS::number> value);
  void setDisclosure(const BPMNOS::number instanceId, const BPMN::Node* node, BPMNOS::number disclosureTime);
  void addPendingDisclosure(const BPMNOS::number instanceId, StochasticPendingDisclosure&& pending);
  void addCompletionExpression(const BPMNOS::number instanceId, const BPMN::Node* task, CompletionExpression&& expr);
  void addArrivalExpression(const BPMNOS::number instanceId, const BPMN::Node* node, ArrivalExpression&& expr);

  /// Get or create RNG for (instance, node) pair
  std::mt19937& getRng(size_t instanceId, const BPMN::Node* node) const;

  mutable std::unordered_map<size_t, InstanceData> instances;
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosure; ///< Instance ID -> Node -> disclosure time
  mutable std::unordered_map<size_t, std::vector<StochasticPendingDisclosure>> pendingDisclosures; ///< Instance ID -> pending disclosures
  mutable std::set<std::pair<size_t, const Attribute*>> disclosedAttributes; ///< Track which attributes have been disclosed

  /// Arrival expressions per (instance, node)
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<ArrivalExpression>>> arrivalExpressions;

  /// Completion expressions per (instance, node)
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<CompletionExpression>>> completionExpressions;

  /// Per (instance, node) RNG for reproducibility
  mutable std::map<std::pair<size_t, const BPMN::Node*>, std::mt19937> rngs;

  unsigned int scenarioSeed;
  BPMNOS::number earliestInstantiationTime;
  BPMNOS::number latestInstantiationTime;

  /// RandomDistributionFactory for expression evaluation (set by provider)
  mutable RandomDistributionFactory* randomFactory = nullptr;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StochasticScenario_H
