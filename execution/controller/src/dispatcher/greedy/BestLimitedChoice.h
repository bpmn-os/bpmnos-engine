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
 * enumerated values and the boundaries for each choice.
 */
class BestLimitedChoice : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BestLimitedChoice(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
private:
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
  void determineAlternatives( std::vector<BPMNOS::Values>& alternatives, const BPMNOS::Model::ExtensionElements* extensionElements, BPMNOS::Values& status,  BPMNOS::Values& data, BPMNOS::Values& globals, BPMNOS::Values& choices, size_t index = 0);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestLimitedChoice_H

