#ifndef BPMNOS_Model_ExpectedValueDataProvider_H
#define BPMNOS_Model_ExpectedValueDataProvider_H

#include "StaticDataProvider.h"
#include "model/utility/src/ExpectedValueFactory.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include <map>

namespace BPMNOS::Model {

class ExpectedValueScenario;

/**
 * @brief Data provider that accepts stochastic CSV format but uses expected values.
 *
 * ExpectedValueDataProvider accepts CSV files with 3, 4, or 5 columns:
 * - 3-column: INSTANCE_ID; NODE_ID; INITIALIZATION
 * - 4-column: INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE
 * - 5-column: INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; COMPLETION
 *
 * Behavior:
 * - DISCLOSURE column is ignored (all values disclosed at time 0)
 * - COMPLETION expressions are evaluated using expected values when tasks complete
 * - Random functions return expected values instead of sampling
 */
class ExpectedValueDataProvider : public StaticDataProvider {
public:
  ExpectedValueDataProvider(const std::string& modelFile, const std::string& instanceFileOrString);
  ExpectedValueDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instanceFileOrString);
  ~ExpectedValueDataProvider() override = default;

  std::unique_ptr<Scenario> createScenario(unsigned int scenarioId = 0) override;

protected:
  void readInstances();
  void readInstancesExtendedFormat(const CSVReader::Table& table, size_t columnCount);
  BPMNOS::number evaluateExpressionWithExpectedValues(const std::string& expression) const;

  ExpectedValueFactory expectedValueFactory;
  mutable LIMEX::Handle<double> expectedValueHandle;
  bool expectedValueHandleInitialized = false;
  void initializeExpectedValueHandle() const;

  /// Stored completion expressions: instanceId -> node -> list of (attribute, expression)
  struct CompletionExprData {
    const Attribute* attribute;
    std::string expressionStr;
  };
  std::map<size_t, std::map<const BPMN::Node*, std::vector<CompletionExprData>>> completionExpressions;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpectedValueDataProvider_H
