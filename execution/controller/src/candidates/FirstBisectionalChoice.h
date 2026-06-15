#ifndef BPMNOS_Execution_FirstBisectionalChoice_H
#define BPMNOS_Execution_FirstBisectionalChoice_H

#include <bpmn++.h>
#include <tuple>
#include <vector>
#include <memory>
#include "model/bpmnos/src/extensionElements/Choice.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Candidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless choice decision candidates for the first feasible pending choice request, using bisection.
 *
 * On each call evaluateCandidates reads systemState->pendingChoiceDecisions and, for each request in turn,
 * calls determineBestChoices; it stops at the first request that yields a feasible choice ("first" refers to
 * the request). determineBestChoices finds the best feasible value of a single attribute with bounds and a
 * multipleOf discretizer by bisection; for multiple choices, an explicit enumeration, or a continuous attribute
 * it evaluates the alternatives by enumeration instead. Every evaluated alternative is kept alive (in
 * evaluatedChoices) until the next evaluation; only the best feasible one is added to the reward-ordered
 * candidates and returned, so the greedy dispatcher takes it while the full alternative set stays available for
 * rollout. Stateless: no caching across calls, so connect does nothing.
 */
class FirstBisectionalChoice : public Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstBisectionalChoice(Evaluator* evaluator);
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);

protected:
  void evaluateCandidates(const SystemState* systemState) override;
  Evaluator* evaluator;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > evaluatedChoices;  ///< Owns every evaluated choice; entries auto-expire with their token/request, in sync with `candidates`.

private:
  /// Evaluates the enumerated alternatives and returns the best feasible one (used when bisection does not apply).
  std::shared_ptr<Decision> bestEnumeratedChoice(std::shared_ptr<const DecisionRequest> request);
  std::shared_ptr<Decision> discreteBisection(std::shared_ptr<const DecisionRequest> request, const BPMNOS::Model::Choice* choice);

  // Helper types
  struct Candidate {
    size_t index;
    std::shared_ptr<ChoiceDecision> decision;

    bool isFeasible() const { return decision && decision->reward().has_value(); }
    double reward() const { return decision->reward().value(); }
  };

  // State for the current request/search (set by determineBestChoices / discreteBisection)
  std::weak_ptr<const Token> token_ptr;              ///< weak token of the request being evaluated, for emplacing candidates
  std::weak_ptr<const DecisionRequest> request_ptr;  ///< weak request being evaluated, for emplacing candidates
  const Token* token;
  std::vector<BPMNOS::number> values;
  Candidate best;

  // Helper methods for discreteBisection
  Candidate evaluate(size_t index);

  // Find any feasible solution using BFS, returns (leftInfeasible, candidate, rightInfeasible)
  std::tuple<size_t, Candidate, size_t> findFeasible(size_t first, size_t last);

  // Search functions for different boundary conditions
  void findBetweenFeasibleAndFeasible(Candidate left, Candidate right);
  void findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIndex);
  void findBetweenInfeasibleAndFeasible(size_t infeasibleIndex, Candidate feasible);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstBisectionalChoice_H
