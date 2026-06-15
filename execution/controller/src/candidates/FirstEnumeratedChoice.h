#ifndef BPMNOS_Execution_FirstEnumeratedChoice_H
#define BPMNOS_Execution_FirstEnumeratedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Candidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless choice decision candidates for the first feasible pending choice request (enumerated).
 *
 * On each call evaluateCandidates reads systemState->pendingChoiceDecisions and, for each request in turn,
 * calls determineBestChoices; it stops at the first request that yields a feasible choice ("first" refers to
 * the request). determineBestChoices enumerates the alternative choices, evaluates each, keeps them all alive
 * (in evaluatedChoices) until the next evaluation, and returns the best feasible one; only that best is added to
 * the reward-ordered candidates, so the greedy dispatcher takes it while the full alternative set stays
 * available for rollout. Stateless: no caching across calls, so connect does nothing.
 */
class FirstEnumeratedChoice : public Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstEnumeratedChoice(Evaluator* evaluator);
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
protected:
  void evaluateCandidates(const SystemState* systemState) override;
  Evaluator* evaluator;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > evaluatedChoices;  ///< Owns every evaluated choice; entries auto-expire with their token/request, in sync with `candidates`.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstEnumeratedChoice_H
