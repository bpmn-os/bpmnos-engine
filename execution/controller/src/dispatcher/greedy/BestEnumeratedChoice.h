#ifndef BPMNOS_Execution_BestEnumeratedChoice_H
#define BPMNOS_Execution_BestEnumeratedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a choice decision for a token at a decision task.
 *
 * The BestEnumeratedChoice dispatcher creates a the best evaluated choice considering
 * enumerated values and the boundaries for each choice.
 */
class BestEnumeratedChoice : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BestEnumeratedChoice(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestEnumeratedChoice_H

