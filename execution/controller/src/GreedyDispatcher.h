#ifndef BPMNOS_Execution_GreedyDispatcher_H
#define BPMNOS_Execution_GreedyDispatcher_H

#include <bpmn++.h>
#include "Evaluator.h"
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/DataUpdate.h"
#include "Decision.h"
#include "DecisionStore.h"

namespace BPMNOS::Execution {

/**
 * @brief Base class for evaluator-based dispatchers, selecting decisions by reward.
 *
 * Holds the pending decisions and their evaluations in a set ordered by reward,
 * partitioned by dependency (invariant / time-dependent / data-dependent) so a decision
 * is re-evaluated only when a clock tick or a relevant data update invalidates it.
 * dispatchEvent evaluates the new decisions and returns the highest-reward feasible event.
 */
// WeakPtrs... are < std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >
template <typename... WeakPtrs>
class GreedyDispatcher : public EventDispatcher, public Observer {
public:
  GreedyDispatcher(Evaluator* evaluator);
  virtual ~GreedyDispatcher() = default;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
protected:
  Evaluator* evaluator;
  DecisionStore<WeakPtrs...> decisionStore;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyDispatcher_H
