#ifndef BPMNOS_Execution_BestLimitedChoice_H
#define BPMNOS_Execution_BestLimitedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a choice decision for a token at a decision task.
 *
 * The BestLimitedChoice dispatcher creates a the best evaluated choice considering
 * enumerated values and the boundaries for a numeric decision.
 * The boundaries and enumeration values are deduced from the attribute type 
 * and restrictions of type @ref BPMNOS::Model::LinearExpression or @ref BPMNOS::Model::Enumeration.
 */
class BestLimitedChoice : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BestLimitedChoice(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
private:
  auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > requestsWithoutDecisions;
  std::vector< BPMNOS::Values > determineDecisions(const Token* token );
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestLimitedChoice_H

