#ifndef BPMNOS_Execution_StochasticDataProvider_H
#define BPMNOS_Execution_StochasticDataProvider_H

#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "DynamicDataProvider.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/RandomDistributionFactory.h"

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
 * the ready expressions of an activity give the status a token arriving at it becomes ready with, once the
 * timestamp of this status is reached, and the completion expressions of a task give the status it
 * completes with. Every random expression of an instance and a node is evaluated with a random number
 * generator of its own.
 *
 * A fork of a run with seed `r` and index `i` has the seed `r + i + 1`. It keeps every initialization
 * disclosed before the instant following the current time of the run, its spawn time, and samples every
 * other initialization and disclosure anew, a timestamp or disclosure time before the spawn time being
 * sampled again up to @ref maxResamplingTries times and then set to the spawn time. The statuses the
 * tokens of the installed state become ready and complete with are sampled anew when the state is
 * installed.
 */
class StochasticDataProvider : public DynamicDataProvider {
public:
  /**
   * @brief Scenario holding a realisation of the instance data.
   */
  /// @brief The number of times a timestamp or disclosure time of a fork before the spawn time is sampled.
  static constexpr int maxResamplingTries = 4;

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
   * @param clockTickDuration Milliseconds of wall-clock time between two clock ticks, zero meaning none.
   */
  StochasticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int seed = 0, unsigned int clockTickDuration = 0);

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
  BPMNOS::Values getActivityReadyStatus(StaticDataProvider::Scenario& scenario, const Token* token) const override;
  BPMNOS::number getActivityReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Values& readyStatus) const override;
  BPMNOS::Values getCompletionStatus(StaticDataProvider::Scenario& scenario, const Token* token) const override;

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

  /// @brief Method compiling an expression of the given column at the node of a row, which may only assign a status attribute.
  std::unique_ptr<BPMNOS::Model::Expression> compileStatusExpression(const InstanceDataReader::Row& row, const std::string& expression, const std::string& column) const;

  /// @brief Method sampling the initializations of a scenario, keeping those of the original scenario of a
  /// fork disclosed before the spawn time.
  void sample(Scenario& scenario, const Scenario* original, BPMNOS::number spawnTime) const;

  /// @brief Method applying the given expressions of an instance and a node to a status.
  void apply(Scenario& scenario, const Expressions& expressions, size_t instanceId, const BPMN::Node* node, BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

  const unsigned int seed;
  mutable BPMNOS::RandomDistributionFactory randomDistributionFactory; ///< The factory of the random functions, whose random number generator is set for every evaluation
  LIMEX::Handle<double> handle; ///< The handle with the lookup tables of the model and the random functions
  std::vector<Initialization> initializations; ///< The initializations in the order of the rows
  Expressions readyExpressions; ///< The ready expressions of each instance and activity
  Expressions completionExpressions; ///< The completion expressions of each instance and task
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StochasticDataProvider_H
