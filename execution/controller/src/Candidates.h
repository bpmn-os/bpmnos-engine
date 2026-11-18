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
#include "Deferrals.h"

namespace BPMNOS::Execution {

class Notifier;

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
 * A collection constructed with the deferrals of a decision maker passes over the candidates of deferred tokens when
 * iterated, so that a dispatcher never meets them; a collection constructed without behaves as if nothing is deferred.
 *
 * WeakPtrs... are the candidate's weak identifiers, the first being the token, e.g. < std::weak_ptr<const Token>,
 * std::weak_ptr<const DecisionRequest> > (and a std::weak_ptr<const Message> for message delivery).
 */
template <typename... WeakPtrs>
class Candidates : public Observer {
public:
  /// The evaluator every candidate of this collection is evaluated through, owned here because this is
  /// where it is read: the collection builds its decisions with it and holds it for as long as it may.
  Candidates(std::shared_ptr<Evaluator> evaluator) : evaluator(std::move(evaluator)) {}
  /// The deferrals are those of the decision maker the collection serves, which they outlive.
  Candidates(std::shared_ptr<Evaluator> evaluator, const Deferrals& deferrals) : evaluator(std::move(evaluator)), deferrals(&deferrals) {}
  virtual ~Candidates() = default;

  /// Subscriptions are made by the specializations; the base does nothing.
  virtual void connect(Notifier* /*notifier*/) {}

  /// Binds the per-run system state (specializations override and call this).
  void notice(const Observable* observable) override {
    if ( observable->getObservableType() == Observable::Type::SystemState ) {
      systemState = static_cast<const SystemState*>(observable);
    }
  }

  using CandidateSet = auto_set< double, descending, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> >;

  /// Iterator over the reward-ordered candidates, passing over those of deferred tokens.
  class iterator {
  public:
    using Underlying = typename CandidateSet::iterator;
    iterator(Underlying current, Underlying last, const Deferrals* deferrals) : current(current), last(last), deferrals(deferrals) { skipDeferred(); }
    iterator& operator++() { ++current; skipDeferred(); return *this; }
    auto operator*() const { return *current; }
    bool operator!=(const iterator& other) const { return current != other.current; }
  private:
    Underlying current;
    Underlying last;
    const Deferrals* deferrals;
    void skipDeferred() {
      if ( !deferrals ) {
        return;
      }
      while ( current != last ) {
        auto token = std::get<1>(*current).lock();
        if ( !token || !deferrals->isDeferred(token.get()) ) {
          return;
        }
        ++current;
      }
    }
  };

  /// Evaluation is lazy — iterating triggers it.
  iterator begin() { evaluateCandidates(); return iterator(candidates.begin(), candidates.end(), deferrals); }
  iterator end() { return iterator(candidates.end(), candidates.end(), deferrals); }

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

  std::shared_ptr<Evaluator> evaluator; ///< Evaluates every candidate; owned here, so a decision built with it is never built through a dangling one.
  const SystemState* systemState = nullptr; ///< Per-run system state (bound by notice).
  const Deferrals* deferrals = nullptr; ///< The deferrals of the decision maker, nullptr if the collection was constructed without
  CandidateSet candidates; ///< Reward-ordered candidate decisions, descending (best first; infeasible last); holds only weak references.
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > decisions; ///< Sole owner of the decisions referenced by `candidates`.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Candidates_H
