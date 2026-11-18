#ifndef BPMNOS_Execution_Choices_H
#define BPMNOS_Execution_Choices_H

#include <bpmn++.h>
#include <optional>
#include <vector>
#include "execution/controller/src/CachedCandidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Choice decision candidates of every pending choice request, ranked by reward.
 *
 * Collects the choice requests via notice and searches the alternatives of every request: the values a choice allows
 * are the alternatives of its enumeration, or the multiples of its step within its bounds, the step of a decimal
 * attribute without a step being the precision of numbers. Given a threshold k, a choice allowing at least k values is
 * searched by bisection and every other choice is enumerated. Without a threshold, every choice with a step, stated or
 * implied by the type of its attribute, is enumerated, and a decimal attribute without a step, whose values are too
 * many to be enumerated, is searched by bisection. The bisection evaluates multiples of the step without enumerating
 * them. The choices of a decision task with several choices are enumerated jointly.
 *
 * Every evaluated alternative is a candidate, so that the front of the reward-ordered set is the best feasible
 * alternative of all requests and the other alternatives stay available for rollout. The evaluations are cached and
 * invalidated by the data updates touching their dependencies. A request is invalidated as a whole: the bounds and
 * the enumeration of a choice are read from the data, and the alternatives a bisection evaluates depend on the rewards
 * it finds, so that evaluating single alternatives anew could keep values no longer allowed or miss better ones. When a
 * decision of a request is invalidated, every decision of the request is dropped and its search is run again. A request
 * whose search yielded no alternative holds no decision that could be invalidated, and is searched again when a data
 * update touches an attribute its choices depend on.
 */
class Choices : public CachedCandidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  /**
   * @param evaluator The evaluator evaluating every alternative
   * @param bisect The least number of values of a choice searched by bisection, none if only the choices of a
   * decimal attribute without a step are searched by bisection
   */
  Choices(std::shared_ptr<Evaluator> evaluator, std::optional<size_t> bisect = std::nullopt);
  void connect(Notifier* notifier) override;
  void notice(const Observable* observable) override;
  /// Clears the cache together with the requests awaiting a search.
  void clear() override;

protected:
  void evaluateCandidates() override;

private:
  std::optional<size_t> bisect;

  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > requestsToSearch; ///< The requests whose alternatives are to be searched, in the order in which they arose
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > requestsWithoutAlternatives; ///< The requests whose search yielded no alternative

  /// Queues a request for a search unless it is queued already.
  void queueSearch(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr);
  /// Drops every decision of the request.
  void dropDecisions(const DecisionRequest* request);
  /// Searches the alternatives of the request anew, every evaluated alternative becoming a candidate.
  void search(std::shared_ptr<const DecisionRequest> request);
  /// Evaluates the alternatives of the request by enumeration or by bisection and returns the best feasible one.
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);

  /// Evaluates the jointly enumerated alternatives of several choices and returns the best feasible one.
  std::shared_ptr<Decision> bestEnumeratedChoices(std::shared_ptr<const DecisionRequest> request);
  /// Evaluates every value of a single choice and returns the best feasible one.
  std::shared_ptr<Decision> bestEnumeratedChoice();
  /// Searches the values of a single choice by bisection and returns the best feasible one found.
  std::shared_ptr<Decision> discreteBisection();

  struct Candidate {
    size_t index;
    std::shared_ptr<ChoiceDecision> decision;

    bool isFeasible() const { return decision && decision->reward().has_value(); }
    double reward() const { return decision->reward().value(); }
  };

  // State of the search of the current request (set by search)
  std::weak_ptr<const Token> token_ptr;              ///< weak token of the request being searched, for listing candidates
  std::weak_ptr<const DecisionRequest> request_ptr;  ///< weak request being searched, for listing candidates
  std::vector<BPMNOS::number> enumeration;           ///< the values of a choice by enumeration
  double firstMultiple;                              ///< the first multiple of the step within the bounds of a choice by bounds
  double step;                                       ///< the step of a choice by bounds
  bool bounded;                                      ///< whether the values are the multiples of the step rather than the enumeration
  size_t count;                                      ///< the number of values the choice allows
  size_t alternatives;                               ///< the number of alternatives evaluated for the request
  Candidate best;

  /// Returns the value at the given position among the values the choice allows.
  BPMNOS::number value(size_t index) const;

  /// Evaluates a decision and lists it as a candidate, owned and indexed for invalidation.
  void addAlternative(std::shared_ptr<ChoiceDecision> decision);

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

#endif // BPMNOS_Execution_Choices_H
