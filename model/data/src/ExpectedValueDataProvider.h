#ifndef BPMNOS_Model_ExpectedValueDataProvider_H
#define BPMNOS_Model_ExpectedValueDataProvider_H

#include "StaticDataProvider.h"
#include "model/utility/src/ExpectedValueFactory.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {

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
 * - COMPLETION column is ignored (operators can be used to compute expected values during execution)
 * - Random functions in INITIALIZATION return expected values instead of sampling
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

  /// Override to use expectedValueHandle instead of model->limexHandle
  BPMNOS::number evaluateExpression(const std::string& expression) const override;

  void initializeExpectedValueHandle();

  ExpectedValueFactory expectedValueFactory;
  LIMEX::Handle<double> expectedValueHandle;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExpectedValueDataProvider_H
