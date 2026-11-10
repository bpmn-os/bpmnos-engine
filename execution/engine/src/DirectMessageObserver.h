#ifndef BPMNOS_Execution_DirectMessageObserver_H
#define BPMNOS_Execution_DirectMessageObserver_H

#include <memory>
#include <bpmn++.h>
#include "Observer.h"
#include "execution/utility/src/auto_list.h"
#include "Message.h"
#include "MessageDeliveryRequest.h"

namespace BPMNOS::Execution {

struct Mediator;
struct Event;

/**
 * @brief Delivers the messages that are directly addressed, which is not a decision.
 *
 * A message is directly addressed to a token awaiting a delivery if either the sender has provided the
 * recipient or the recipient has provided the sender, so that the message follows from its sending, as a
 * catching event consumes the event it waits for. The observer supplies one delivery at a time, which the engine
 * dispatches before the events of the controller, so that the delivery is notified and processed like any other
 * event. The delivery of any other message is decided by the controller.
 */
class DirectMessageObserver : public Observer {
public:
  DirectMessageObserver();
  /**
   * @brief Subscribes the observer to the messages, the delivery requests and the system states of the engine.
   */
  void connect(Mediator* mediator);
  void notice(const Observable* observable) override;
  /**
   * @brief Returns the delivery of a directly addressed message, or a null pointer if there is none.
   */
  std::shared_ptr<Event> dispatchEvent();
private:
  /// Requests awaiting a delivery, each with the messages found to match it. The header a request is
  /// matched by is the request's own, so it is not kept here.
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const MessageDeliveryRequest>, auto_list< std::weak_ptr<const Message> > > messageDeliveryRequests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DirectMessageObserver_H
