#ifndef BPMNOS_Execution_FirstMatchingMessageDelivery_H
#define BPMNOS_Execution_FirstMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Message.h"

namespace BPMNOS::Execution {

/**
 * @brief Dispatches the first matching message delivery decision, without evaluation.
 *
 * A message delivery request can be fulfilled by a created @ref Message when its origin is an
 * admissible sender and its header matches the recipient header. The first matching message
 * delivery decision is dispatched.
 */
class FirstMatchingMessageDelivery : public EventDispatcher, public Observer {
public:
  FirstMatchingMessageDelivery();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  using EventDispatcher::notice;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, auto_list< std::weak_ptr<const Message> >, const BPMNOS::Values > messageDeliveryRequests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstMatchingMessageDelivery_H

