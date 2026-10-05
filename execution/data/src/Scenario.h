#ifndef BPMNOS_Execution_Scenario_H
#define BPMNOS_Execution_Scenario_H

#include <memory>
#include <chrono>

namespace BPMNOS::Execution {

class DataProvider;

/**
 * @brief Base class of the scenarios the data providers create.
 *
 * A scenario holds the realisation of the data a run is executed against and whatever its data provider
 * records about the run. Only the data provider that created it reads or changes it, so the base holds
 * nothing but that data provider.
 */
class Scenario {
public:
  virtual ~Scenario() = default;

  const std::shared_ptr<const DataProvider> dataProvider; ///< The data provider that created the scenario

  std::chrono::steady_clock::time_point previousClockTick; ///< Wall-clock time at which the previous clock tick was enqueued
protected:
  Scenario(std::shared_ptr<const DataProvider> dataProvider);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Scenario_H
