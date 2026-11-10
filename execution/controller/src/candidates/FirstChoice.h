#ifndef BPMNOS_Execution_FirstChoice_H
#define BPMNOS_Execution_FirstChoice_H

#include <bpmn++.h>
#include <tuple>
#include <vector>
#include <memory>
#include <optional>
#include "model/bpmnos/src/extensionElements/Choice.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Candidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless choice decision candidates for the first feasible pending choice request.
 *
 * On each call evaluateCandidates reads systemState->pendingChoiceDecisions and, for each request in turn,
 * calls determineBestChoices; it stops at the first request that yields a feasible choice ("first" refers to
 * the request). The values a choice allows are the alternatives of its enumeration, or the multiples of its step
 * within its bounds, the step of a decimal attribute without a step being the precision of numbers. Given a
 * threshold k, a choice allowing at least k values is searched by bisection and every other choice is enumerated.
 * Without a threshold, every choice with a step, stated or implied by the type of its attribute, is enumerated, and
 * a decimal attribute without a step, whose values are too many to be enumerated, is searched by bisection. The
 * bisection evaluates multiples of the step without enumerating them. The choices of a decision task with several
 * choices are enumerated jointly. Every evaluated alternative is added to the reward-ordered candidates (which own
 * them until the next evaluation), and the best feasible one is returned — so the greedy dispatcher takes the front
 * (best) while the evaluated alternatives stay available for rollout.
 * Stateless: nothing is cached across iterations; connect only binds the per-run system state.
 */
class FirstChoice : public Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  /**
   * @param evaluator The evaluator evaluating every alternative
   * @param bisect The least number of values of a choice searched by bisection, none if only the choices of a
   * decimal attribute without a step are searched by bisection
   */
  FirstChoice(std::shared_ptr<Evaluator> evaluator, std::optional<size_t> bisect = std::nullopt);
  void connect(Notifier* notifier) override;
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);

protected:
  void evaluateCandidates() override;

private:
  std::optional<size_t> bisect;

  /// Evaluates the jointly enumerated alternatives of several choices and returns the best feasible one.
  std::shared_ptr<Decision> bestEnumeratedChoices(std::shared_ptr<const DecisionRequest> request);
  /// Evaluates every value of a single choice and returns the best feasible one.
  std::shared_ptr<Decision> bestEnumeratedChoice();
  std::shared_ptr<Decision> discreteBisection();

  // Helper types
  struct Candidate {
    size_t index;
    std::shared_ptr<ChoiceDecision> decision;

    bool isFeasible() const { return decision && decision->reward().has_value(); }
    double reward() const { return decision->reward().value(); }
  };

  // State for the current request/search (set by determineBestChoices)
  std::weak_ptr<const Token> token_ptr;              ///< weak token of the request being evaluated, for emplacing candidates
  std::weak_ptr<const DecisionRequest> request_ptr;  ///< weak request being evaluated, for emplacing candidates
  std::vector<BPMNOS::number> enumeration;           ///< the values of a choice by enumeration
  double firstMultiple;                              ///< the first multiple of the step within the bounds of a choice by bounds
  double step;                                       ///< the step of a choice by bounds
  bool bounded;                                      ///< whether the values are the multiples of the step rather than the enumeration
  size_t count;                                      ///< the number of values the choice allows
  Candidate best;

  /// Returns the value at the given position among the values the choice allows.
  BPMNOS::number value(size_t index) const;

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

#endif // BPMNOS_Execution_FirstChoice_H
