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
// WeakPtrs... are < std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >
template <typename... WeakPtrs>
class GreedyDispatcher : public EventDispatcher, public Observer {
public:
  GreedyDispatcher(Evaluator* evaluator);
  virtual ~GreedyDispatcher() = default;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  BPMNOS::number timestamp;
protected:
  Evaluator* evaluator;
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > decisionsWithoutEvaluation;

  void addEvaluation(WeakPtrs..., std::shared_ptr<Decision> decision, std::optional<double> reward );

  auto_set< double, WeakPtrs..., std::weak_ptr<Event> > evaluatedDecisions;  
  virtual void dataUpdate(const DataUpdate* update);
  bool intersect(const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) const;
  void removeObsolete(const DataUpdate* update, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& evaluation, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);
  void removeDependentEvaluations(const DataUpdate* update, std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > >& evaluatedDecisions, auto_list< WeakPtrs..., std::shared_ptr<Decision> >& unevaluatedDecisions);

  auto_list< WeakPtrs..., std::shared_ptr<Decision> > invariantEvaluations;
  auto_list< WeakPtrs..., std::shared_ptr<Decision> > timeDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > dataDependentEvaluations;
  std::unordered_map< long unsigned int, auto_list< WeakPtrs..., std::shared_ptr<Decision> > > timeAndDataDependentEvaluations;
  void clockTick();
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GreedyDispatcher_H

