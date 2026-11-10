#ifndef BPMNOS_Execution_FirstFeasibleEntry_H
#define BPMNOS_Execution_FirstFeasibleEntry_H

#include <bpmn++.h>
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Entry decision candidates dispatched as the first feasible entry.
 *
 * Collects entry requests via notice, each of which is one of a child of a sequential ad-hoc subprocess,
 * the engine itself entering any other activity; evaluateCandidates creates and evaluates the pending entry
 * decisions only until the first feasible one, which is then the front of the reward-ordered set. Unlike
 * SequentialEntries, it does not rank the candidates by their reward.
 */
class FirstFeasibleEntry : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstFeasibleEntry(std::shared_ptr<Evaluator> evaluator);
  void connect(Notifier* notifier) override;
  void notice(const Observable* observable) override;
protected:
  void evaluateCandidates() override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstFeasibleEntry_H
