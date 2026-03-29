#ifndef BPMNOS_Execution_TaskCompletionHandler_H
#define BPMNOS_Execution_TaskCompletionHandler_H

#include <bpmn++.h>
#include "EventDispatcher.h"
#include "Observer.h"
#include "execution/utility/src/auto_list.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Class dispatching a completion event when task completion time is reached.
 *
 * Subscribes to Token observable, filters for BUSY state at Task nodes
 * (excluding SendTask, ReceiveTask, DecisionTask which complete via external events).
 * Uses getTaskCompletionStatus() to check if completion time is reached.
 */
class TaskCompletionHandler : public EventDispatcher, public Observer {
public:
  TaskCompletionHandler();
  ~TaskCompletionHandler();

  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState) override;

private:
  std::shared_ptr<Event> getEvent(const Token* token, const SystemState* systemState);

  auto_list<std::weak_ptr<Token>> awaitingTokens;
  auto_list<std::weak_ptr<Token>, std::shared_ptr<Event>> pendingEvents;
  BPMNOS::number lastCheckedTime;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TaskCompletionHandler_H
