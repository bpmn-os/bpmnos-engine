#ifndef BPMNOS_Execution_FirstEnumeratedChoice_H
#define BPMNOS_Execution_FirstEnumeratedChoice_H

#include <bpmn++.h>
#include "execution/engine/src/DecisionRequest.h"
#include "execution/controller/src/Candidates.h"
#include "execution/controller/src/Evaluator.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Stateless choice decision candidates for the first feasible pending choice request (enumerated).
 *
 * On each call evaluateCandidates reads systemState->pendingChoiceDecisions and, for each request in turn,
 * determines the best feasible choice via determineBestChoices; it stops at the first request that yields a
 * feasible choice and adds it, leaving the remaining requests pending ("first" refers to the request).
 * determineBestChoices enumerates the alternative choices, evaluates each, and returns the best feasible
 * one. Stateless: no collection or caching, so connect does nothing.
 */
class FirstEnumeratedChoice : public Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  FirstEnumeratedChoice(Evaluator* evaluator);
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);
protected:
  void evaluateCandidates(const SystemState* systemState) override;
  Evaluator* evaluator;
  std::shared_ptr<Decision> bestChoice;  ///< Owns the selected choice so its weak_ptr in `candidates` stays valid for the dispatcher.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstEnumeratedChoice_H
