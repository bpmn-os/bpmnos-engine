#ifndef BPMNOS_Execution_BestFirstExit_H
#define BPMNOS_Execution_BestFirstExit_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/decisions/ExitDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class BestFirstExit : public GreedyDispatcher {
public:
  BestFirstExit( std::function<std::optional<double>(const Event* event)> evaluator = &ExitDecision::localEvaluator);
//  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;

private:
//  std::function< std::optional<double>(const Event* event) > evaluator;
//  auto_set< double, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<ExitDecision> > decisions;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstExit_H

