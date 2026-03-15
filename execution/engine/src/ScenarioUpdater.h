#ifndef BPMNOS_Execution_ScenarioUpdater_H
#define BPMNOS_Execution_ScenarioUpdater_H

#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

class Engine;

/**
 * @brief Observer that updates scenario state during execution.
 *
 * - On ClockTick: calls scenario->revealData() for deferred disclosure
 * - On Token ARRIVED/CREATED at Activity: calls scenario->initializeArrivalData()
 * - On Token BUSY at Task: calls scenario->setTaskCompletionStatus()
 *
 * Works with Static, Dynamic, and Stochastic scenarios.
 */
class ScenarioUpdater : public Observer {
public:
  ScenarioUpdater() = default;

  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ScenarioUpdater_H
