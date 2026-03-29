#ifndef BPMNOS_Execution_ReadyHandler_H
#define BPMNOS_Execution_ReadyHandler_H

#include <bpmn++.h>
#include "EventDispatcher.h"
#include "Observer.h"
#include "execution/utility/src/auto_list.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

class Token;

/**
 * @brief Class dispatching a ready event when the required data is available for a token at an activity.
 *
 * Subscribes to Token observable, filters for ARRIVED state at Activity nodes.
 * Uses getActivityReadyStatus() to check if activity data is disclosed.
 */
class ReadyHandler : public EventDispatcher, public Observer {
public:
  ReadyHandler();
  ~ReadyHandler();

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

#endif // BPMNOS_Execution_ReadyHandler_H

