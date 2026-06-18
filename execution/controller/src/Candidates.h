#ifndef BPMNOS_Execution_Candidates_H
#define BPMNOS_Execution_Candidates_H

#include <bpmn++.h>
#include <memory>
#include <limits>
#include "execution/utility/src/auto_set.h"
#include "execution/utility/src/auto_list.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/Observable.h"
#include "Decision.h"

namespace BPMNOS::Execution {

class Mediator;

/**
 * @brief An iterable collection of evaluated candidate decisions, ordered by reward (best first; infeasible last).
 *
 * Iterating (`for ( auto c : candidates )`) evaluates the candidates for the current state — captured from the
 * SystemState notice — and yields them best first. `candidates` holds only weak references (each decision's Event
 * and Evaluation); `decisions` owns them, so a candidate drops once its Evaluation is reset (the re-evaluation
 * signal) or its decision dies; `addDecision`/`addCandidate` couple listing with ownership. A specialization
 * implements evaluateCandidates and decides how many to create — a first-feasible one only the first feasible, a
 * best-of-all one all of them.
 *
 * WeakPtrs... are the candidate's weak identifiers, e.g. < std::weak_ptr<const Token>,
 * std::weak_ptr<const DecisionRequest> > (and a std::weak_ptr<const Message> for message delivery).
 */
template <typename... WeakPtrs>
class Candidates : public Observer {
public:
  virtual ~Candidates() = default;

  /// Subscriptions are made by the specializations; the base does nothing.
  virtual void connect(Mediator* /*mediator*/) {}

  /// Binds the per-run system state (specializations override and call this).
  void notice(const Observable* observable) override {
    if ( observable->getObservableType() == Observable::Type::SystemState ) {
      systemState = static_cast<const SystemState*>(observable);
    }
  }

  /// Evaluation is lazy — iterating triggers it.
  auto begin() { evaluateCandidates(); return candidates.begin(); }
  auto end() { return candidates.end(); }

protected:
  /// Specialization hook: produce the candidates for the current `systemState`.
  virtual void evaluateCandidates() = 0;

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

  const SystemState* systemState = nullptr; ///< Per-run system state (bound by notice).
  auto_set< double, descending, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> > candidates; ///< Reward-ordered candidate decisions, descending (best first; infeasible last); holds only weak references.
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > decisions; ///< Sole owner of the decisions referenced by `candidates`.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Candidates_H
