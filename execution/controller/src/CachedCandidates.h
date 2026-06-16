#ifndef BPMNOS_Execution_CachedCandidates_H
#define BPMNOS_Execution_CachedCandidates_H

#include <bpmn++.h>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <unordered_map>
#include "execution/utility/src/auto_list.h"
#include "execution/utility/src/auto_set.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/DataUpdate.h"
#include "Decision.h"
#include "Candidates.h"

namespace BPMNOS::Execution {

/**
 * @brief Abstract base class for cached local evaluations of candidate decisions of decision requests.
 *
 * Collects pending decisions and keeps each evaluation in the reward-ordered `candidates` list until a clock
 * tick or a relevant data update invalidates it, so unchanged decisions are not re-evaluated. The base owns
 * every decision in `decisions`; the lists here hold only weak references that track which decisions await
 * evaluation and which must be re-evaluated when time or referenced data change. Concrete per-type sources
 * derive this and implement `evaluateCandidates` — collecting candidates via `notice`/`addDecision` and
 * evaluating them via `evaluateDecisions`/`addEvaluation`.
 */
// WeakPtrs... are < std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >
template <typename... WeakPtrs>
class CachedCandidates : public Observer, public Candidates<WeakPtrs...> {
public:
  CachedCandidates();

  /// Callback evaluating one pending decision; returns an event to dispatch (stopping the evaluation) or nullptr to continue.
  using Evaluate = std::function< std::shared_ptr<Event>(WeakPtrs..., std::shared_ptr<Decision>) >;

  /// Register a yet-unevaluated decision: take ownership in the base `decisions` and queue a weak reference for evaluation.
  void addDecision(WeakPtrs..., std::shared_ptr<Decision> decision);
  /// List an already-owned, evaluated decision in the reward-ordered `candidates` and weak-index it under its dependency category.
  void addEvaluation(WeakPtrs..., std::shared_ptr<Decision> decision);
  /// Evaluate the pending decisions in turn (removing each); return the first event a callback dispatches, or nullptr.
  std::shared_ptr<Event> evaluateDecisions(const Evaluate& evaluate);
  /// Advance to currentTime; if the clock ticked, re-issue the time-dependent decisions for re-evaluation.
  void advanceTime(BPMNOS::number currentTime);
  /// Handle an observable: a DataUpdate invalidates the affected evaluations.
  void notice(const Observable* observable) override;

private:
  BPMNOS::number timestamp; ///< Time of the most recent clock tick applied.
  void clockTick(); ///< Re-issue the time-dependent decisions for re-evaluation.
  void dataUpdate(const DataUpdate* update);
  bool intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const;
  void removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& unevaluatedDecisions);
  void removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::weak_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::weak_ptr<Decision> >& unevaluatedDecisions);
  /// Invoke `evaluate` with the candidate's weak identifiers and the re-locked decision.
  template <std::size_t... I>
  std::shared_ptr<Event> applyEvaluate(const Evaluate& evaluate, const std::tuple< WeakPtrs..., std::weak_ptr<Decision> >& tuple, std::shared_ptr<Decision> decision, std::index_sequence<I...>);

  auto_list< WeakPtrs..., std::weak_ptr<Decision> > candidatesWithoutEvaluations; ///< Weak index of decisions awaiting evaluation.
  auto_list< WeakPtrs..., std::weak_ptr<Decision> > timeDependentEvaluations;     ///< Weak index of evaluated time-dependent decisions.
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::weak_ptr<Decision> > > dataDependentEvaluations;        ///< Per-instance weak index of evaluated data-dependent decisions.
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::weak_ptr<Decision> > > timeAndDataDependentEvaluations; ///< Per-instance weak index of evaluated time-and-data-dependent decisions.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CachedCandidates_H
