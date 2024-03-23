#ifndef BPMNOS_Execution_BestFirstParallelEntry_H
#define BPMNOS_Execution_BestFirstParallelEntry_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class BestFirstParallelEntry : public EventDispatcher, public Observer {
public:
  BestFirstParallelEntry( std::function<std::optional<double>(const Event* event)> evaluator = &EntryDecision::localEvaluator);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;

private:
  std::function< std::optional<double>(const Event* event) > evaluator;
  auto_set< double, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<EntryDecision> > decisions;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstParallelEntry_H

