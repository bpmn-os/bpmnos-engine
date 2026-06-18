#ifndef BPMNOS_Execution_MessageDeliveries_H
#define BPMNOS_Execution_MessageDeliveries_H

#include <bpmn++.h>
#include "execution/engine/src/Message.h"
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Message delivery decision candidates, ranked by reward.
 *
 * A message delivery request can be fulfilled by a created @ref Message whose origin is an admissible
 * sender and whose header matches the recipient header. notice collects the matching request/message
 * pairs; evaluateCandidates evaluates each (after updating the recipient's status with the message), so
 * the front of the reward-ordered set is the best feasible delivery (best-of-all, for the competing layer).
 */
class MessageDeliveries : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message> > {
public:
  MessageDeliveries(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  void clear() override;
protected:
  void evaluateCandidates(const SystemState* systemState) override;
  Evaluator* evaluator;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, const BPMNOS::Values > requests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveries_H
