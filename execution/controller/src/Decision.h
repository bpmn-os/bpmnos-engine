#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "Evaluation.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

class Evaluator;
struct DecisionRequest;

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : virtual public Event {
public:
  Decision(const DecisionRequest* request, Evaluator* evaluator);

  virtual std::shared_ptr<Evaluation> evaluate() = 0; ///< Evaluates the reward for the decision. Returns null if decision is infeasible.
  std::shared_ptr<Evaluation> evaluation;  ///< Current evaluation (used for lazy removal from evaluatedDecisions)

  std::weak_ptr<const DecisionRequest> request; ///< The decision request this decision responds to; expires when the request is reset/superseded/withdrawn.

  /// Stale if the token no longer exists or the decision request was superseded (reset/withdrawn).
  bool expired() const override;

  std::optional<double> reward() const {
    return evaluation ? evaluation->value : std::nullopt;
  }

  bool timeDependent;
  std::set<const BPMNOS::Model::Attribute*> dataDependencies;
protected:
  void determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies);
  Evaluator* evaluator;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
