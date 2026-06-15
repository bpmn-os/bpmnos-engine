#ifndef BPMNOS_Execution_BestEnumeratedChoice_H
#define BPMNOS_Execution_BestEnumeratedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless dispatcher creating the best evaluated choice for a token at a decision task.
 *
 * Reads systemState->pendingChoiceDecisions on each dispatch; for each choice request it considers
 * the enumerated values and bounds for each choice, evaluates the alternatives, and dispatches the
 * best feasible choice. A request for which no feasible choice exists is left to the
 * MyopicDecisionTaskTerminator.
 */
class BestEnumeratedChoice : public EventDispatcher {
public:
  BestEnumeratedChoice(Evaluator* evaluator);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
protected:
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BestEnumeratedChoice_H
