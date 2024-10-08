#ifndef BPMNOS_Execution_MyopicMessageTaskTerminator_H
#define BPMNOS_Execution_MyopicMessageTaskTerminator_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an error event for a token at a message task if the message can not be delivered.
 *
 * The MyopicMessageTaskTerminator terminates a @ref BPMN:ReceiveTask or a @ref BPMN:SendTask with an error if
 * the respective message cannot be delivered. It only raises an error if all tokens have advanced as far as possible,
 * i.e., if all pending decisions except for message delivery decisions are made, and if no task waits for a 
 * @ref BPMNOS::Execution::ReadyEvent or a @ref BPMNOS::Execution::CompletionEvent, and if no @ref BPMN::TimerCatchEvent
 * has a token. It assumes that a message handler making possible message delivery decisions has been called before,
 * so that no message can currently be delivered. The handler is myopic and does not consider future instantiations
 * of processes.
 */
class MyopicMessageTaskTerminator : public EventDispatcher, public Observer {
public:
  MyopicMessageTaskTerminator();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > messageDeliveryDecisions;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > otherDecisions;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MyopicMessageTaskTerminator_H

