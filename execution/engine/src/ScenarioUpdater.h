#ifndef BPMNOS_Execution_ScenarioUpdater_H
#define BPMNOS_Execution_ScenarioUpdater_H

#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

class Engine;

/**
 * @brief Observer that updates scenario state during execution.
 *
 * - On ClockTick: calls scenario->noticeClockTick() with the time the clock is advancing to
 * - On Token ARRIVED/CREATED at Activity: calls scenario->noticeReadyPending()
 * - On Token BUSY at Task: calls scenario->noticeCompletionPending()
 *
 * These are the three notifications a scenario receives. Each is declared on Scenario and defaults to
 * doing nothing, so every scenario is notified and none is required to react.
 */
class ScenarioUpdater : public Observer {
public:
  ScenarioUpdater() = default;

  void subscribe(Engine* engine);
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ScenarioUpdater_H
