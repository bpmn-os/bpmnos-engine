#ifndef BPMNOS_Execution_BestMatchingMessageDelivery_H
#define BPMNOS_Execution_BestMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Message.h"
#include "execution/controller/src/decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
class BestMatchingMessageDelivery : public EventDispatcher, public Observer {
public:
  BestMatchingMessageDelivery( std::function<std::optional<double>(const Event* event)> evaluator = &MessageDeliveryDecision::localEvaluator);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
private:
  std::function< std::optional<double>(const Event* event) > evaluator;
  auto_set< double, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<const Message>, std::shared_ptr<MessageDeliveryDecision> > decisions;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, const BPMNOS::Values > requests;
  auto_list< std::weak_ptr<const Message> > messages;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestMatchingMessageDelivery_H

