#ifndef BPMNOS_Execution_BestMatchingMessageDelivery_H
#define BPMNOS_Execution_BestMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/Message.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
class BestMatchingMessageDelivery : public GreedyDispatcher {
public:
  BestMatchingMessageDelivery(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, const BPMNOS::Values > requests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestMatchingMessageDelivery_H

