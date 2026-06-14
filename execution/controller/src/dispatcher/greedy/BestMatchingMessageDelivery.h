#ifndef BPMNOS_Execution_BestMatchingMessageDelivery_H
#define BPMNOS_Execution_BestMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/Message.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Dispatches the message delivery decision with the best evaluation after updating the recipient's status.
 *
 * A message delivery request can be fulfilled by a created @ref Message when its origin is an
 * admissible sender and its header matches the recipient header. Each fulfilling decision is
 * evaluated after updating the recipient's status with the message, and the one with the best
 * evaluation is dispatched.
 */
class BestMatchingMessageDelivery : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> > {
public:
  BestMatchingMessageDelivery(Evaluator* evaluator);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, const BPMNOS::Values > requests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestMatchingMessageDelivery_H

