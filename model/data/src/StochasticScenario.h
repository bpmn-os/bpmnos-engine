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
  const Attribute* attribute;              ///< The attribute to initialize
  BPMNOS::number disclosureTime;           ///< Own disclosure time (from expression)
  BPMNOS::number value;                    ///< Pre-computed value to reveal at disclosure time
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
 * - Scenario copying with resampling of future values
 *
 * @par Resampling Behavior
 * When a scenario is copied at a given spawnTime, values with disclosure time >= spawnTime
 * are re-evaluated using a new RNG seed. If re-evaluated timestamps or disclosure times
 * fall before spawnTime, they are resampled up to @ref maxResamplingTries times. If still
 * in the past after all attempts, they fall back to spawnTime.
 *
 * @warning Resampling with fallback may alter the effective probability distribution,
 *          creating a point mass at spawnTime.
 *
 * @par Assumptions
 * - CSV rows must be ordered with parent nodes before child nodes for dependent expressions
 */
class StochasticDataProvider;

class StochasticScenario : public Scenario {
  friend class StochasticDataProvider;

public:
  /**
   * @brief Maximum number of resampling attempts when re-evaluated disclosure time is before spawnTime
   */
  static constexpr int maxResamplingTries = 4;

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

  /**
   * @brief House keeping of internal data.
   *
   * This method only does house keeping without actually revealing data.
   * Disclosures of data are handled by checking node-level disclosureTimes.
   */
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

  /**
   * @brief (Re-)evaluates deferred disclosures with disclosure time >= spawnTime.
   *
   * Timestamps and disclosure times that fall before spawnTime are resampled up to
   * maxResamplingTries times, falling back to spawnTime if still in the past.
   *
   * @param spawnTime Items with disclosure time < spawnTime are not evaluated again
   */
  void evaluateDeferredDisclosures(BPMNOS::number spawnTime = std::numeric_limits<BPMNOS::number>::lowest());

  /// Get or create RNG for (instance, node) pair
  std::mt19937& getRng(size_t instanceId, const BPMN::Node* node) const;

  mutable std::unordered_map<size_t, InstanceData> instances;
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosureTimes; ///< Instance ID -> Node -> disclosure time
  mutable std::unordered_map<size_t, std::unordered_map<const Attribute*, StochasticPendingDisclosure>> pastDisclosures; ///< Already disclosed
  mutable std::unordered_map<size_t, std::unordered_map<const Attribute*, StochasticPendingDisclosure>> pendingDisclosures; ///< Not yet disclosed

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
