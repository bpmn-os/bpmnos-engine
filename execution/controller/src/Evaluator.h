#ifndef BPMNOS_Execution_Evaluator_H
#define BPMNOS_Execution_Evaluator_H

#include <bpmn++.h>
#include "decisions/EntryDecision.h"
#include "decisions/ExitDecision.h"
#include "decisions/ChoiceDecision.h"
#include "decisions/MessageDeliveryDecision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending Evaluator
 */
class Evaluator {
public:
  virtual std::optional<double> evaluate(EntryDecision* decision) = 0;
  virtual std::optional<double> evaluate(ExitDecision* decision) = 0;
  virtual std::optional<double> evaluate(ChoiceDecision* decision) = 0;
  virtual std::optional<double> evaluate(MessageDeliveryDecision* decision) = 0;

  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(EntryDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(ExitDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(ChoiceDecision* decision) = 0;
  virtual std::set<const BPMNOS::Model::Attribute*> getDependencies(MessageDeliveryDecision* decision) = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Evaluator_H
