#ifndef BPMNOS_Execution_CachedCandidates_H
#define BPMNOS_Execution_CachedCandidates_H

#include <bpmn++.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include "execution/utility/src/auto_list.h"
#include "execution/utility/src/auto_set.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/DataUpdate.h"
#include "Decision.h"
#include "Candidates.h"

namespace BPMNOS::Execution {

/**
 * @brief bstract base class for cached local evaluations of candidate decisions of decision requests.
 *
 * Collects pending decisions and keeps each evaluation in the reward-ordered `candidates` list until a
 * clock tick or a relevant data update invalidates it, so unchanged decisions are not re-evaluated.
 * Concrete per-type sources derive this and implement `createCandidates` — collecting candidates via
 * `notice`/`addDecision` and evaluating them via `evaluateDecisions`/`addEvaluation`.
 */
// WeakPtrs... are < std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >
template <typename... WeakPtrs>
class CachedCandidates : public Observer, public Candidates<WeakPtrs...> {
public:
  CachedCandidates();

  /// Callback evaluating one pending decision; returns an event to dispatch (stopping the evaluation) or nullptr to continue.
  using Evaluate = std::function< std::shared_ptr<Event>(WeakPtrs..., std::shared_ptr<Decision>) >;

  /// Register an unevaluated candidate decision.
  void addDecision(WeakPtrs..., std::shared_ptr<Decision> decision);
  /// Insert an already-evaluated decision into the reward-ordered `candidates` and file it under its dependency category.
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
  void removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);
  void removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);

  auto_list< WeakPtrs..., std::shared_ptr<Decision> > candidatesWithoutEvaluations; ///< Collected decisions awaiting evaluation.
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > invariantEvaluations;
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > timeDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > dataDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > timeAndDataDependentEvaluations;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CachedCandidates_H
