#ifndef BPMNOS_Execution_Environment_H
#define BPMNOS_Execution_Environment_H

#include <bpmn++.h>
#include "EventDispatcher.h"
#include "Observer.h"
#include "execution/utility/src/auto_list.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Class connecting a run to the scenario it executes.
 *
 * The environment observes the engine and dispatches the events the scenario supplies. It reports the
 * progress of the run to the scenario:
 * - On ClockTick: calls scenario->noticeClockTick() with the time the clock is advancing to
 * - On Token ARRIVED/CREATED at Activity: calls scenario->noticeReadyPending()
 * - On Token READY at Activity: calls scenario->noticeReady()
 * - On Token BUSY at Task: calls scenario->noticeCompletionPending()
 * - On Token COMPLETED at Task: calls scenario->noticeCompletion()
 *
 * Each notification is declared on Scenario and defaults to doing nothing, so every scenario is notified
 * and none is required to react.
 *
 * It dispatches a ready event for the token at a created process instance once the scenario discloses
 * the status of the process at its instantiation time, as determined by getProcessReadyStatus() and
 * getDataAttributes(), before any ready event for an activity at the same instant, a ready event for a
 * token that has arrived at an activity once the scenario discloses the status and data the activity requires, as
 * determined by getActivityReadyStatus() and getDataAttributes(), and a completion event for a token that is busy at a task once the scenario
 * discloses its completion status, as determined by getTaskCompletionStatus(). Send, receive and
 * decision tasks are excluded, since they complete through other events.
 */
class Environment : public EventDispatcher, public Observer {
public:
  Environment();

  void connect(Mediator* mediator) override;
  using EventDispatcher::notice;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;

private:
  std::shared_ptr<Event> getReadyEvent(const Token* token, const SystemState* systemState);
  std::shared_ptr<Event> dispatchReadyEvent(const SystemState* systemState);
  std::shared_ptr<Event> getCompletionEvent(const Token* token, const SystemState* systemState);
  std::shared_ptr<Event> dispatchCompletionEvent(const SystemState* systemState);

  auto_list<std::weak_ptr<Token>> processTokensAwaitingReadyEvent; ///< Tokens at created process instances, checked before those at activities
  auto_list<std::weak_ptr<Token>> tokensAwaitingReadyEvent;
  auto_list<std::weak_ptr<Token>, std::shared_ptr<Event>> pendingReadyEvents;
  BPMNOS::number lastReadyCheckTime;

  auto_list<std::weak_ptr<Token>> tokensAwaitingCompletionEvent;
  auto_list<std::weak_ptr<Token>, std::shared_ptr<Event>> pendingCompletionEvents;
  BPMNOS::number lastCompletionCheckTime;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Environment_H
