#ifndef BPMNOS_Execution_FirstMatchingMessageDelivery_H
#define BPMNOS_Execution_FirstMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
class FirstMatchingMessageDelivery : public EventDispatcher, public Observer {
public:
  FirstMatchingMessageDelivery();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, const BPMNOS::Values > messageDeliveryRequests;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstMatchingMessageDelivery_H

