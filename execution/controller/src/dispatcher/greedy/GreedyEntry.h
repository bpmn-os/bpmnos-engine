#ifndef BPMNOS_Execution_GreedyEntry_H
#define BPMNOS_Execution_GreedyEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the first feasible entry event for a token awaiting the entry at an activity or
 * the best token awaiting the entry at an activity performed by a sequential performer 
 */
class GreedyEntry : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  GreedyEntry(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyEntry_H

