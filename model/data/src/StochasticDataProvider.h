#ifndef BPMNOS_Model_StochasticDataProvider_H
#define BPMNOS_Model_StochasticDataProvider_H

#include <bpmn++.h>
#include <limex.h>
#include "DataProvider.h"
#include "StochasticScenario.h"
#include "model/utility/src/CSVReader.h"
#include "model/utility/src/RandomDistributionFactory.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <memory>
#include <vector>

namespace BPMNOS::Model {

/**
 * @brief Data provider supporting stochastic behavior.
 *
 * StochasticDataProvider supports:
 * - 3-column format (INSTANCE_ID, NODE_ID, INITIALIZATION) - static behavior
 * - 4-column format (+ DISCLOSURE) - dynamic behavior
 * - 6-column format (+ READY, COMPLETION) - stochastic behavior
 *
 * Has its own LIMEX handle with lookup tables from Model plus random functions.
 *
 * @note CSV ordering: For dependent expressions, parent node rows must be listed
 *       before child node rows in the CSV file. If a child expression depends on
 *       a parent's deferred attribute, the parent must be evaluated first.
 *
 * @note Guidance constraint: Guidance expressions must only reference attributes
 *       that are disclosed at the time of access. This is not enforced; it is
 *       the user's responsibility to ensure correct usage.
 */
class StochasticDataProvider : public DataProvider {
public:
  StochasticDataProvider(const std::string& modelFile, const std::string& instanceFileOrString,
                         unsigned int seed = 0);
  StochasticDataProvider(const std::string& modelFile, const std::vector<std::string>& folders,
                         const std::string& instanceFileOrString, unsigned int seed = 0);
  ~StochasticDataProvider() override = default;

  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;

  /// Get the stochastic LIMEX handle (for expression compilation)
  const LIMEX::Handle<double>& getStochasticHandle() const { return stochasticHandle; }

protected:
  CSVReader reader;
  unsigned int seed;
  size_t columnCount;

  /// Dedicated LIMEX handle with lookup tables + random functions
  LIMEX::Handle<double> stochasticHandle;

  /// Random distribution factory
  RandomDistributionFactory randomFactory;

  void initializeStochasticHandle();
  void readInstances();

  struct StochasticInstanceData {
    const BPMN::Process* process;
    BPMNOS::number id;
    BPMNOS::number instantiation;
    std::unordered_map<const Attribute*, BPMNOS::number> data;
  };

  /// Deferred attribute with expressions to evaluate per-scenario
  struct DeferredAttribute {
    const Attribute* attribute;
    const BPMN::Node* node;
    std::shared_ptr<Expression> initializationExpression;
    std::shared_ptr<Expression> disclosureExpression;
  };

  std::unordered_map<long unsigned int, StochasticInstanceData> instances;

  /// Deferred attributes (evaluated per-scenario)
  std::unordered_map<size_t, std::vector<DeferredAttribute>> deferredAttributes;

  /// Ready expressions per (instance, node) - shared with scenarios
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<std::shared_ptr<Expression>>>> readyExpressions;

  /// Completion expressions per (instance, node) - shared with scenarios
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, std::vector<std::shared_ptr<Expression>>>> completionExpressions;

  /// Node disclosure times
  std::unordered_map<size_t, std::unordered_map<const BPMN::Node*, BPMNOS::number>> disclosureTimes;

  BPMNOS::number evaluateExpression(const std::string& expressionString) const override;
  BPMNOS::number evaluateExpression(size_t instanceId, const BPMN::Node* node,
                                     const std::string& expressionString,
                                     ValueType type) const override;
  void evaluateGlobal(const std::string& initializationString) override;
  BPMNOS::number getEffectiveDisclosure(size_t instanceId, const BPMN::Node* node, BPMNOS::number ownDisclosure);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_StochasticDataProvider_H
