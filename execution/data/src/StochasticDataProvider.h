#ifndef BPMNOS_Execution_StochasticDataProvider_H
#define BPMNOS_Execution_StochasticDataProvider_H

#include <limits>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "DynamicDataProvider.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "RandomDistributionFactory.h"

namespace BPMNOS::Execution {

/**
 * @brief Data provider sampling the instance data of a run from random expressions.
 *
 * The data is read from a CSV table with the columns `INSTANCE_ID`, `NODE_ID`, `INITIALIZATION`,
 * `DISCLOSURE`, `READY` and `COMPLETION`, of which the table may omit the trailing ones. The expressions may
 * use random functions. The global values are sampled once, with the seed of the data provider. Every
 * initialization and disclosure of an instance is sampled for each scenario, with the seed of the
 * scenario, which is the seed of the data provider plus the realisation the scenario is created for, in the
 * order of the rows. A run on a scenario proceeds as on the dynamic data provider with the values and
 * disclosure times sampled, but what is disclosed is an announcement, which the realisation may contradict:
 * the ready expressions of an activity give the values a token arriving at it becomes ready with, and the
 * completion expressions of a task give the status it completes with. A ready expression assigning the
 * timestamp must be the first ready expression of its node and may read only what the scope inherits, the
 * status the token holds, the data of the enclosing instances and the globals: it is sampled when the token
 * arrives and determines when the token becomes ready. The other ready expressions are applied when the token
 * becomes ready, over the initial values of the scope instance, and may assign a status attribute or a data
 * attribute the scope owns. Every random expression of an instance and a node is evaluated with a random number
 * generator of its own, seeded from the seed of the scenario and the identifiers of the instance and the
 * node, so that a realisation depends on nothing but the seed, the model and the instance data.
 *
 * A fork of a run with seed `r` and index `i` has the seed `r + i + 1`. It keeps what the run has revealed
 * and samples everything else anew. The initializations are disclosed with clock ticks, so that the fork
 * of a run at time `t` keeps every initialization disclosed by `t` and samples every other one anew, its
 * timestamp and disclosure time being sampled again up to @ref maxResamplingTries times while they precede
 * `t + 1` and then set to `t + 1`. The tokens awaiting their ready or completion event in the installed
 * system state are those whose values have not been revealed: the ready timestamp of a token awaiting its ready
 * event and the completion status of a token awaiting its completion event are computed anew when the state is
 * installed, a timestamp being sampled again up to @ref maxResamplingTries times while it precedes `t`, if an
 * expression assigns it, and then set to `t`. A timestamp of `t` is admissible,
 * since an event of an instant may follow a decision of that instant. A status computed for a token arriving
 * at an activity or becoming busy at a task is computed as in any run.
 */
class StochasticDataProvider : public DynamicDataProvider {
public:
  /// @brief The number of times a timestamp or disclosure time sampled anew for a fork is sampled at most.
  static constexpr int maxResamplingTries = 4;

  /**
   * @brief Scenario holding a realisation of the instance data.
   */
  class Scenario : public StaticDataProvider::Scenario {
  public:
    Scenario(std::shared_ptr<const StochasticDataProvider> dataProvider, unsigned int seed);

    const unsigned int seed; ///< The seed of the realisation
    std::unordered_map<size_t, std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>> values; ///< The values sampled for each instance
    std::unordered_map<size_t, BPMNOS::number> instantiationTimes; ///< The instantiation time sampled for each instance
    std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosureTimes; ///< The disclosure times sampled for each instance
    std::vector<BPMNOS::number> initializationDisclosureTimes; ///< The disclosure time sampled for each initialization, in the order of the rows

    /// @brief Method returning the random number generator of an instance and a node.
    std::mt19937& getRandomNumberGenerator(size_t instanceId, const BPMN::Node* node);

  private:
    std::map<std::pair<size_t, const BPMN::Node*>, std::mt19937> randomNumberGenerators;
  };

  /**
   * @param model The model the data provider is built on.
   * @param instanceFileOrString The name of the CSV file holding the instance data or its content.
   * @param seed The seed of the data provider.
   * @param clockTickDuration Wall-clock time between two clock ticks, zero advancing time at once and
   *        std::chrono::milliseconds::max() never.
   */
  StochasticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int seed = 0, std::chrono::milliseconds clockTickDuration = std::chrono::milliseconds::zero());

  /**
   * @brief Method creating the scenario of a run for the given realisation, whose seed is the seed of the
   * data provider plus the realisation.
   */
  std::unique_ptr<Execution::Scenario> createScenario(unsigned int realisation = 0) const override;

  std::unique_ptr<Execution::Scenario> forkScenario(const Execution::Scenario& scenario, unsigned int index) const override;

protected:
  void readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle) override;
  const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& getInstanceValues(const StaticDataProvider::Scenario& scenario, size_t instanceId) const override;
  const std::unordered_map<const BPMN::Node*, BPMNOS::number>& getDisclosureTimes(const StaticDataProvider::Scenario& scenario, size_t instanceId) const override;
  BPMNOS::number getProcessReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId) const override;
  std::optional<BPMNOS::number> getReadyTimestamp(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const override;
  BPMNOS::number getActivityReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId, const BPMN::Node* activity, std::optional<BPMNOS::number> readyTimestamp) const override;
  void applyReadyValues(StaticDataProvider::Scenario& scenario, const Token* token, InstanceValues& values) const override;
  BPMNOS::Status getCompletionStatus(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const override;

private:
  /**
   * @brief Initialization of an attribute of an instance, sampled for each scenario.
   */
  struct Initialization {
    size_t instanceId;
    const BPMN::Node* node;
    const BPMNOS::Model::Attribute* attribute;
    std::unique_ptr<BPMNOS::Model::Expression> value;
    std::unique_ptr<BPMNOS::Model::Expression> disclosure;
  };

  using Expressions = std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<std::unique_ptr<BPMNOS::Model::Expression>>>>;

  /// @brief Method compiling a ready expression at the node of a row, which may assign a status attribute or a data
  /// attribute the scope of the node owns, and which, if it assigns the timestamp, must be the first ready expression
  /// of the node and may read only what the scope inherits.
  std::unique_ptr<BPMNOS::Model::Expression> compileReadyExpression(const InstanceDataReader::Row& row, const std::string& expression) const;

  /// @brief Method compiling a completion expression at the node of a row, which may only assign a status attribute.
  std::unique_ptr<BPMNOS::Model::Expression> compileCompletionExpression(const InstanceDataReader::Row& row, const std::string& expression) const;

  /// @brief Method returning the ready expressions of an instance and a node, nullptr if there are none.
  const std::vector<std::unique_ptr<BPMNOS::Model::Expression>>* getReadyExpressions(size_t instanceId, const BPMN::Node* node) const;

  /// @brief Method sampling the initializations of a scenario, keeping those of the original scenario of a
  /// fork disclosed before the spawn time.
  void sample(Scenario& scenario, const Scenario* original, BPMNOS::number spawnTime) const;

  /// @brief Method computing a completion status by evaluating the completion expressions of an instance and a node on it,
  /// sampling it again while its timestamp precedes the given time if an expression assigns the timestamp,
  /// and setting the timestamp to the given time at last.
  void computeStatus(Scenario& scenario, size_t instanceId, const BPMN::Node* node, BPMNOS::Status& status, const BPMNOS::SharedData& data, BPMNOS::number earliest) const;

  const unsigned int seed;
  mutable RandomDistributionFactory randomDistributionFactory; ///< The factory of the random functions, whose random number generator is set for every evaluation
  LIMEX::Handle<double> handle; ///< The handle with the lookup tables of the model and the random functions
  std::vector<Initialization> initializations; ///< The initializations in the order of the rows
  Expressions readyExpressions; ///< The ready expressions of each instance and activity
  Expressions completionExpressions; ///< The completion expressions of each instance and task
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StochasticDataProvider_H
