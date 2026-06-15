#ifndef BPMNOS_Execution_DecisionStore_H
#define BPMNOS_Execution_DecisionStore_H

#include <bpmn++.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include "execution/utility/src/auto_list.h"
#include "execution/utility/src/auto_set.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/DataUpdate.h"
#include "Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Store of candidate decisions including their cached local evaluations.
 *
 * Holds the pending decisions and local evaluations in a set ordered by reward.
 * Evaluations are stored as long as they are not invalidated due to a clock tick or
 * a relevant data update.
 */
// WeakPtrs... are < std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >
template <typename... WeakPtrs>
class DecisionStore : public Observer {
public:
  DecisionStore();

  /// Callback evaluating one pending decision; returns an event to dispatch (stopping the evaluation) or nullptr to continue.
  using Evaluate = std::function< std::shared_ptr<Event>(WeakPtrs..., std::shared_ptr<Decision>) >;

  /// Register an unevaluated candidate decision.
  void addDecision(WeakPtrs..., std::shared_ptr<Decision> decision);
  /// Insert an already-evaluated decision into evaluatedDecisions and file it under its dependency category.
  void addEvaluation(WeakPtrs..., std::shared_ptr<Decision> decision);
  /// Evaluate the pending decisions in turn (removing each); return the first event a callback dispatches, or nullptr.
  std::shared_ptr<Event> evaluateDecisions(const Evaluate& evaluate);
  /// Advance to currentTime; if the clock ticked, re-issue the time-dependent decisions for re-evaluation.
  void advanceTime(BPMNOS::number currentTime);
  /// Handle an observable forwarded by the owning dispatcher: a DataUpdate invalidates the affected evaluations.
  void notice(const Observable* observable) override;
  /// Get the best evaluated decision if it is feasible, nullptr otherwise.
  std::shared_ptr<Event> getBestDecision();
  /// Read-only view of the evaluated decisions, ordered by reward (best first; infeasible last).
  const auto_set< double, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> >& getEvaluatedDecisions() const;

private:
  BPMNOS::number timestamp; ///< Time of the most recent clock tick applied.
  void clockTick(); ///< Re-issue the time-dependent decisions for re-evaluation.
  void dataUpdate(const DataUpdate* update);
  bool intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const;
  void removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);
  void removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);

  auto_list< WeakPtrs..., std::shared_ptr<Decision> > decisionsWithoutEvaluation; ///< Collected decisions awaiting evaluation.
  auto_set< double, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> > evaluatedDecisions; ///< Evaluated decisions ordered by negated reward (best first; infeasible last).
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > invariantEvaluations;
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > timeDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > dataDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > timeAndDataDependentEvaluations;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DecisionStore_H
