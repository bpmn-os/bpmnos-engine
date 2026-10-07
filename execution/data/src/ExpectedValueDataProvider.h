#ifndef BPMNOS_Execution_ExpectedValueDataProvider_H
#define BPMNOS_Execution_ExpectedValueDataProvider_H

#include <memory>
#include <string>
#include "StaticDataProvider.h"

namespace BPMNOS::Execution {

/**
 * @brief Data provider replacing every random expression of the instance data by its expected value.
 *
 * The data is read from a CSV table with the columns `INSTANCE_ID`, `NODE_ID`, `INITIALIZATION`,
 * `DISCLOSURE`, `READY` and `COMPLETION`, of which the table may omit the trailing ones. Every initialization
 * is evaluated with the expected values of the random functions, and the further columns are ignored, so that
 * all data is known from the start of a run, which proceeds as on the static data provider.
 */
class ExpectedValueDataProvider : public StaticDataProvider {
public:
  /**
   * @param model The model the data provider is built on.
   * @param instanceFileOrString The name of the CSV file holding the instance data or its content.
   * @param clockTickDuration Wall-clock time between two clock ticks, zero advancing time at once and
   *        std::chrono::milliseconds::max() never.
   */
  ExpectedValueDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, std::chrono::milliseconds clockTickDuration = std::chrono::milliseconds::zero());
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ExpectedValueDataProvider_H
