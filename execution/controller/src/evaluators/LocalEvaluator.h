#ifndef BPMNOS_Execution_LocalEvaluator_H
#define BPMNOS_Execution_LocalEvaluator_H

#include <bpmn++.h>
#include "execution/controller/src/Evaluator.h"

namespace BPMNOS::Execution {

/**
 * @brief Class using local evaluations to determine the reward of a decision.
 */
class LocalEvaluator : public Evaluator {
public:
  virtual bool updateValues(EntryDecision* decision, Values& status, Values& data, Values& globals);
  virtual bool updateValues(ExitDecision* decision, Values& status, Values& data, Values& globals);
  virtual bool updateValues(ChoiceDecision* decision, Values& status, Values& data, Values& globals);
  virtual bool updateValues(MessageDeliveryDecision* decision, Values& status, Values& data, Values& globals);

  std::optional<double> evaluate(EntryDecision* decision) override;
  std::optional<double> evaluate(ExitDecision* decision) override;
  std::optional<double> evaluate(ChoiceDecision* decision) override;
  std::optional<double> evaluate(MessageDeliveryDecision* decision) override;

  std::set<const BPMNOS::Model::Attribute*> getDependencies(EntryDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ExitDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ChoiceDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(MessageDeliveryDecision* decision) override;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_LocalEvaluator_H
