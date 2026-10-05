#include "Scenario.h"

using namespace BPMNOS::Execution;

Scenario::Scenario(std::shared_ptr<const DataProvider> dataProvider)
  : dataProvider(std::move(dataProvider))
  , previousClockTick(std::chrono::steady_clock::now())
{
}
