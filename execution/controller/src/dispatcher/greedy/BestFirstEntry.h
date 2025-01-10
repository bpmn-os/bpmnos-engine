#ifndef BPMNOS_Execution_BestFirstEntry_H
#define BPMNOS_Execution_BestFirstEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching the best entry event for a token awaiting the entry at an activity.
 */
class BestFirstEntry : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BestFirstEntry(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstEntry_H

