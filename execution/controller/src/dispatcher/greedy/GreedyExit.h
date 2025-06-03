#ifndef BPMNOS_Execution_GreedyExit_H
#define BPMNOS_Execution_GreedyExit_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ExitDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the first feasible exit event for a token awaiting the exit at an activity.
 */
class GreedyExit : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  GreedyExit(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyExit_H

