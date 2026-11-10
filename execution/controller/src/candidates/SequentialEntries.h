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
 * Collects the entry requests via notice, each of which is one of a SequentialAdHocSubProcess child, the
 * engine itself entering any other activity; evaluateCandidates creates and evaluates all of them, so the front of
 * the reward-ordered set is the best feasible sequential entry (best-of-all, for the competing layer).
 */
class SequentialEntries : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  SequentialEntries(std::shared_ptr<Evaluator> evaluator);
  void connect(Notifier* notifier) override;
  void notice(const Observable* observable) override;
protected:
  void evaluateCandidates() override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SequentialEntries_H
