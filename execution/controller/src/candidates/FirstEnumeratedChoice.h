#ifndef BPMNOS_Execution_FirstEnumeratedChoice_H
#define BPMNOS_Execution_FirstEnumeratedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Candidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless choice decision candidates for the first feasible pending choice request (enumerated).
 *
 * On each call evaluateCandidates reads systemState->pendingChoiceDecisions and, for each request in turn,
 * calls determineBestChoices; it stops at the first request that yields a feasible choice ("first" refers to
 * the request). determineBestChoices enumerates the alternative choices, evaluates each, adds them all to the
 * reward-ordered candidates (which own them until the next evaluation), and returns the best feasible one — so
 * the greedy dispatcher takes the front (best) while the full alternative set stays available for rollout.
 * Stateless: no caching across calls, so connect does nothing.
 */
class FirstEnumeratedChoice : public Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstEnumeratedChoice(Evaluator* evaluator);
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
protected:
  void evaluateCandidates(const SystemState* systemState) override;
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstEnumeratedChoice_H
