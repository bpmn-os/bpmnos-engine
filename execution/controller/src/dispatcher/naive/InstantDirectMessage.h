#ifndef BPMNOS_Execution_InstantDirectMessage_H
#define BPMNOS_Execution_InstantDirectMessage_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Message.h"

namespace BPMNOS::Execution {

/**
 * @brief Dispatches the first explicitly addressed message delivery decision, without evaluation.
 *
 * Dispatches messages if either the recipient is explicitly provided by the sender or if the sender
 * is explicitly provided by the recipient. The first such message delivery decision is dispatched.
 */
class InstantDirectMessage : public EventDispatcher, public Observer {
public:
  InstantDirectMessage();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, auto_list< std::weak_ptr<const Message> >, const BPMNOS::Values > messageDeliveryRequests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantDirectMessage_H
