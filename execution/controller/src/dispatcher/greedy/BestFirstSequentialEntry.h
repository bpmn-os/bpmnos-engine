#ifndef BPMNOS_Execution_BestFirstSequentialEntry_H
#define BPMNOS_Execution_BestFirstSequentialEntry_H

#include <bpmn++.h>
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/engine/src/SequentialPerformerUpdate.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class BestFirstSequentialEntry : public GreedyDispatcher {
public:
  BestFirstSequentialEntry(Evaluator* evaluator);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;

  void entryRequest(const DecisionRequest* request);
  void sequentialPerformerUpdate(const SequentialPerformerUpdate* update);
protected:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > pendingDecisionsWithoutEvaluation;
  auto_set< double, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<Event> > pendingEvaluatedDecisions;  
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestFirstSequentialEntry_H

