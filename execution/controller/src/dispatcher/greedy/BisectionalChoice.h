#ifndef BPMNOS_Execution_BisectionalChoice_H
#define BPMNOS_Execution_BisectionalChoice_H

#include <bpmn++.h>
#include <functional>
#include <tuple>
#include "model/bpmnos/src/extensionElements/Choice.h"
#include "execution/engine/src/EventDispatcher.h"
#include "execution/controller/src/GreedyDispatcher.h"
#include "execution/controller/src/decisions/ChoiceDecision.h"
#include "BestEnumeratedChoice.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a choice decision for a token at a decision task using bisection.
 *
 * The BisectionalChoice dispatcher uses bisection for attributes with bounds and a multipleOf 
 * discretizer. For attributes with explicit enumeration, enumeration is used.
 */
class BisectionalChoice : public GreedyDispatcher< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> > {
public:
  BisectionalChoice(Evaluator* evaluator);
  void connect(Mediator* mediator) override;
  void notice(const Observable* observable) override;
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  std::shared_ptr<Decision> determineBestChoices(std::shared_ptr<const DecisionRequest> request);

private:
  BestEnumeratedChoice enumeratedChoice;
  std::shared_ptr<Decision> discreteBisection(std::shared_ptr<const DecisionRequest> request, const BPMNOS::Model::Choice* choice);

  // Helper types
  struct Candidate {
    size_t idx;
    std::shared_ptr<ChoiceDecision> decision;

    bool isFeasible() const { return decision && decision->reward.has_value(); }
    double reward() const { return decision->reward.value(); }
  };

  // State for current search (set by discreteBisection)
  const Token* token;
  std::vector<BPMNOS::number> values;
  Candidate best;

  // Helper methods for discreteBisection
  Candidate evaluate(size_t idx);

  // Find any feasible solution using BFS, returns (leftInfeasible, candidate, rightInfeasible)
  std::tuple<size_t, Candidate, size_t> findFeasible(size_t lo, size_t hi);

  // Search functions for different boundary conditions
  void findBetweenFeasibleAndFeasible(Candidate left, Candidate right);
  void findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIdx);
  void findBetweenInfeasibleAndFeasible(size_t infeasibleIdx, Candidate feasible);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_BisectionalChoice_H

