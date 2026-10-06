#ifndef BPMNOS_Execution_LegacyDataProvider_H
#define BPMNOS_Execution_LegacyDataProvider_H

#include <list>
#include <limits>
#include <bpmn++.h>
#include "DataProvider.h"
#include "Scenario.h"
#include "execution/utility/src/auto_list.h"
#include "model/utility/src/Number.h"
#include "model/data/src/Scenario.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Transitional data provider serving a @ref BPMNOS::Model::Scenario "Model::Scenario" through the
 * queue of the environment.
 *
 * Its scenario refers to the wrapped scenario without owning it and holds what is recorded about the run.
 * It reports the progress of the run to the wrapped scenario:
 * - On ClockTick: calls noticeClockTick() with the time the clock is advancing to
 * - On Token ARRIVED/CREATED at Activity: calls noticeReadyPending()
 * - On Token READY at Activity: calls noticeReady()
 * - On Token BUSY at Task: calls noticeCompletionPending()
 * - On Token COMPLETED at Task: calls noticeCompletion()
 *
 * Each notification is declared on Model::Scenario and defaults to doing nothing, so every scenario is
 * notified and none is required to react.
 *
 * Asked to dispatch an event, it enqueues at most one event, determined in the following
 * order. An instantiation event for every process instance becoming known, as determined by
 * getKnownInstantiations(), comes before any other event. A ready event for the token at a created process
 * instance follows once the scenario discloses the status of the process at its instantiation time, as
 * determined by getProcessReadyStatus() and getData(), before any ready event for an activity at
 * the same instant. A ready event for a token that has arrived at an activity follows once the scenario
 * discloses the status and data the activity requires, as determined by getActivityReadyStatus() and
 * getData(), and a completion event for a token that is busy at a task once the scenario
 * discloses its completion status, as determined by getTaskCompletionStatus(). Send, receive and decision
 * tasks are excluded, since they complete through other events. A signal broadcast event for every signal
 * the scenario reports, as determined by getSignals(), comes once no other event is due at the same
 * instant.
 *
 * Asked to advance, it enqueues a clock tick as specified for every data provider. Asked to dispatch an
 * event for the first time after a clock tick has been processed, it enqueues a termination event instead of anything
 * else if the system state is no longer alive, that is, if the scenario is completed and no instance is
 * left.
 */
class LegacyDataProvider : public DataProvider {
public:
  /**
   * @brief Scenario wrapping a Model::Scenario and holding what is recorded about the run.
   */
  class Scenario : public Execution::Scenario {
  public:
    Scenario(std::shared_ptr<const LegacyDataProvider> dataProvider, const BPMNOS::Model::Scenario* scenario);

    const BPMNOS::Model::Scenario* const scenario; ///< The wrapped scenario, which is not owned

    std::list<std::shared_ptr<Event>> pendingInstantiationEvents; ///< Instantiation events determined but not yet enqueued
    BPMNOS::number previousTime; ///< Time the state held before the last clock tick, after which instances become known
    bool instantiationsDue; ///< Whether the instances becoming known at the current time are yet to be determined
    std::list<std::shared_ptr<Event>> pendingSignalBroadcastEvents; ///< Signal broadcast events determined but not yet enqueued
    bool signalsDue; ///< Whether the signals raised at the current time are yet to be determined

    auto_list<std::weak_ptr<Token>> processTokensAwaitingReadyEvent; ///< Tokens at created process instances, checked before those at activities
    auto_list<std::weak_ptr<Token>> tokensAwaitingReadyEvent;
    auto_list<std::weak_ptr<Token>, std::shared_ptr<Event>> pendingReadyEvents;
    BPMNOS::number lastReadyCheckTime;

    auto_list<std::weak_ptr<Token>> tokensAwaitingCompletionEvent;
    auto_list<std::weak_ptr<Token>, std::shared_ptr<Event>> pendingCompletionEvents;
    BPMNOS::number lastCompletionCheckTime;

    bool clockTickProcessed; ///< Whether a clock tick has been processed since events were last dispatched
  };

  /**
   * @param model The model of the scenarios the data provider wraps.
   * @param clockTickDuration Milliseconds of wall-clock time between two clock ticks, zero meaning none.
   * @param endTime The time at which a run ends once nothing is left to do.
   */
  LegacyDataProvider(const BPMNOS::Model::Model* model, unsigned int clockTickDuration = 0, BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Method creating a scenario wrapping the given scenario, which must outlive it, together with the
   * data provider it is held by.
   */
  static std::unique_ptr<Scenario> wrap(const BPMNOS::Model::Scenario* scenario, BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Method creating a scenario of this data provider wrapping the given scenario, which must outlive
   * it. The data provider must be held through a shared pointer.
   */
  std::unique_ptr<Scenario> createScenario(const BPMNOS::Model::Scenario* scenario) const;

  BPMNOS::Values getGlobals(const Execution::Scenario& scenario) const override;
  BPMNOS::number getEarliestInstantiationTime(const Execution::Scenario& scenario) const override;

  void notice(const Observable* observable, Execution::Scenario& scenario, EventQueue& queue) const override;
  void advance(const SystemState* systemState, Execution::Scenario& scenario, EventQueue& queue) const override;
  void dispatchEvent(const SystemState* systemState, Execution::Scenario& scenario, EventQueue& queue) const override;

  /**
   * @brief Method returning true while the wrapped scenario is not completed or an instance is left.
   */
  static bool isAlive(const SystemState* systemState, const Scenario& scenario);

private:
  const BPMNOS::number endTime; ///< The time at which a run ends once nothing is left to do

  std::shared_ptr<Event> determineEvent(const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> dispatchInstantiationEvent(const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> dispatchSignalBroadcastEvent(const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> getReadyEvent(const Token* token, const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> dispatchReadyEvent(const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> getCompletionEvent(const Token* token, const SystemState* systemState, Scenario& scenario) const;
  std::shared_ptr<Event> dispatchCompletionEvent(const SystemState* systemState, Scenario& scenario) const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_LegacyDataProvider_H
