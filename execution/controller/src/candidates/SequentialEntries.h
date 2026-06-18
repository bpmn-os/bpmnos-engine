#ifndef BPMNOS_Execution_SequentialEntries_H
#define BPMNOS_Execution_SequentialEntries_H

#include <bpmn++.h>
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Entry decision candidates for activities of a SequentialAdHocSubProcess, ranked by reward.
 *
 * Collects only entry requests of SequentialAdHocSubProcess children via notice (the remaining entries
 * are left to FirstFeasibleEntry); evaluateCandidates creates and evaluates all of them, so the front of
 * the reward-ordered set is the best feasible sequential entry (best-of-all, for the competing layer).
 */
class SequentialEntries : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  SequentialEntries(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
protected:
  void evaluateCandidates() override;
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SequentialEntries_H
