#ifndef BPMNOS_Execution_BestFirstParallelEntry_H
#define BPMNOS_Execution_BestFirstParallelEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/EntryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class BestFirstParallelEntry : public GreedyDispatcher {
public:
  BestFirstParallelEntry(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstParallelEntry_H

