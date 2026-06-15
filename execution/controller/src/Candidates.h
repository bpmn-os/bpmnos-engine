#ifndef BPMNOS_Execution_Candidates_H
#define BPMNOS_Execution_Candidates_H

#include <bpmn++.h>
#include <memory>
#include "execution/utility/src/auto_set.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/SystemState.h"
#include "Decision.h"

namespace BPMNOS::Execution {

class Mediator;

/**
 * @brief Abstract base for the evaluated candidate decisions of decision requests.
 *
 * Owns the reward-ordered candidate list (best first; infeasible last). A specialization populates it
 * in evaluateCandidates — creating, evaluating, and inserting candidates and deciding how many to create:
 * a first-feasible source creates only the first feasible one (trivially the best in the list); a
 * best-of-all source creates them all. A policy dispatcher calls evaluateCandidates and then reads
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
  const auto_set< double, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> >& getCandidates(const SystemState* systemState) { evaluateCandidates(systemState); return candidates; }

protected:
  /// Create, evaluate, and insert the candidate decisions for the current state into `candidates`, deciding how many to create.
  virtual void evaluateCandidates(const SystemState* systemState) = 0;
  auto_set< double, WeakPtrs..., std::weak_ptr<Event>, std::weak_ptr<Evaluation> > candidates; ///< Reward-ordered candidate decisions (best first; infeasible last).
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Candidates_H
