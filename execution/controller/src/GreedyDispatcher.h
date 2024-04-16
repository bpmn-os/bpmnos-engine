#ifndef BPMNOS_Execution_GreedyDispatcher_H
#define BPMNOS_Execution_GreedyDispatcher_H

#include <bpmn++.h>
#include "Evaluator.h"
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/Observer.h"
#include "execution/engine/src/DataUpdate.h"
#include "Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class for dispatching the event with the best evaluation.
 */
class GreedyDispatcher : public EventDispatcher, public Observer {
public:
  GreedyDispatcher(Evaluator* evaluator);
  virtual ~GreedyDispatcher() = default;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;

protected:
  Evaluator* evaluator;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > decisionsWithoutEvaluation;

  void evaluate(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision);

  auto_set< double, std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<Event> > evaluatedDecisions;  
private:
  void dataUpdate(const DataUpdate* update);
  void clockTick();

  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > invariantEvaluations;
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > timeDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > > dataDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > > timeAndDataDependentEvaluations;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyDispatcher_H

