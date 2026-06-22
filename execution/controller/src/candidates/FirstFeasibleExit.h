#ifndef BPMNOS_Execution_FirstFeasibleExit_H
#define BPMNOS_Execution_FirstFeasibleExit_H

#include <bpmn++.h>
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ExitDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Exit decision candidates dispatched greedily as the first feasible exit.
 *
 * Collects exit requests via notice; evaluateCandidates creates and evaluates the pending exit decisions
 * only until the first feasible one. Exits do not compete, so that lone feasible candidate is the
 * best in the reward-ordered set.
 */
class FirstFeasibleExit : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstFeasibleExit(Evaluator* evaluator);
  void connect(Notifier* notifier) override;
  void notice(const Observable* observable) override;
protected:
  void evaluateCandidates() override;
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstFeasibleExit_H
