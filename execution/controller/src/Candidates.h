#ifndef BPMNOS_Execution_Candidates_H
#define BPMNOS_Execution_Candidates_H

#include <bpmn++.h>
#include <memory>
#include <limits>
#include "execution/utility/src/auto_set.h"
#include "execution/utility/src/auto_list.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/SystemState.h"
#include "Decision.h"

namespace BPMNOS::Execution {

class Mediator;

/**
 * @brief Abstract base for the evaluated candidate decisions of decision requests.
 *
 * Holds a reward-ordered list of candidates (best first; infeasible last) and owns the decisions it refers to.
 * Each candidate entry keeps only weak references to its decision (as an Event) and its Evaluation: it stays as
 * long as the decision is alive and drops by itself once the Evaluation is reset, which signals that the decision
 * must be re-evaluated. `decisions` is the sole owner that keeps every referenced decision alive, and the
 * `addDecision`/`addCandidate` helpers couple insertion with ownership so a candidate cannot be listed without an owner.
 *
 * A specialization populates the candidates in evaluateCandidates — creating, evaluating, and inserting candidates
 * and deciding how many to create: a first-feasible source creates only the first feasible one (trivially the best
 * in the list); a best-of-all source creates them all. A policy dispatcher calls evaluateCandidates and then reads
 * getCandidates, deciding which candidate to dispatch.
 *
 * WeakPtrs... are the candidate's weak identifiers, e.g. < std::weak_ptr<const Token>,
 * std::weak_ptr<const DecisionRequest> > (and a std::weak_ptr<const Message> for message delivery).
 */
template <typename... WeakPtrs>
class Candidates {
public:
  virtual ~Candidates() = default;

  /// Connect to the engine: stateful sources subscribe to their request type and DataUpdate; stateless sources do nothing.
  virtual void connect(Mediator* /*mediator*/) {}

  /// Populate the candidates for the current state (via evaluateCandidates) and return them, ordered by reward (best first; infeasible last).
  const auto_set< double, descending, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> >& getCandidates(const SystemState* systemState) { evaluateCandidates(systemState); return candidates; }

protected:
  /// Create, evaluate, and insert the candidate decisions for the current state into `candidates`, deciding how many to create.
  virtual void evaluateCandidates(const SystemState* systemState) = 0;

  /// Take ownership of a decision without listing it as a candidate. Touching `begin()` lazily erases the expired front of `decisions`.
  void addDecision(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
    decisions.emplace_back(weak_ptrs..., std::move(decision));
    decisions.begin();
  }

  /// Take ownership of a decision and list it as a candidate in one step (descending by reward, best first; infeasible (-infinity) last; cannot list a candidate without an owner).
  void addCandidate(WeakPtrs... weak_ptrs, std::shared_ptr<Decision> decision) {
    auto reward = decision->reward();
    candidates.emplace( reward.has_value() ? (double)reward.value() : -std::numeric_limits<double>::infinity(),
                        weak_ptrs..., decision->weak_from_this(), decision->evaluation );
    addDecision(weak_ptrs..., std::move(decision));
  }

  /// Drop all owned decisions and the candidates referring to them (per-call reset for stateless sources).
  void clearDecisions() {
    candidates.clear();
    decisions.clear();
  }

  auto_set< double, descending, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> > candidates; ///< Reward-ordered candidate decisions, descending (best first; infeasible last); holds only weak references.
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > decisions; ///< Sole owner of the decisions referenced by `candidates`.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Candidates_H
