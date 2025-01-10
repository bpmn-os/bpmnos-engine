#ifndef BPMNOS_Execution_BestFirstExit_H
#define BPMNOS_Execution_BestFirstExit_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ExitDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the best exit event for a token awaiting the exit at an activity.
 */
class BestFirstExit : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BestFirstExit(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstExit_H

