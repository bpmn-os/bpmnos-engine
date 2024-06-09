#ifndef BPMNOS_Execution_BestMatchingMessageDelivery_H
#define BPMNOS_Execution_BestMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/Message.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a message delivery.
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

