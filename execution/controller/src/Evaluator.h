#ifndef BPMNOS_Execution_Evaluator_H
#define BPMNOS_Execution_Evaluator_H

#include <bpmn++.h>
#include "Evaluation.h"
#include "decisions/EntryDecision.h"
#include "decisions/ExitDecision.h"
#include "decisions/ChoiceDecision.h"
#include "decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for an evaluator determining feasibility and reward of a decision
 */
class Evaluator {
public:
  virtual std::shared_ptr<Evaluation> evaluate(EntryDecision* decision) = 0;
  virtual std::shared_ptr<Evaluation> evaluate(ExitDecision* decision) = 0;
  virtual std::shared_ptr<Evaluation> evaluate(ChoiceDecision* decision) = 0;
  virtual std::shared_ptr<Evaluation> evaluate(MessageDeliveryDecision* decision) = 0;

  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(EntryDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(ExitDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(ChoiceDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(MessageDeliveryDecision* decision) = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Evaluator_H
