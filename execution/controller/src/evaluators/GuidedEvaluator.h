#ifndef BPMNOS_Execution_GuidedEvaluator_H
#define BPMNOS_Execution_GuidedEvaluator_H

#include <bpmn++.h>
#include "LocalEvaluator.h"

namespace BPMNOS::Execution {

/**
 * @brief Class using the guidance that may be provided to determine the reward of a decision.
 */
class GuidedEvaluator : public LocalEvaluator {
public:
  bool updateValues(EntryDecision* decision, Values& status, Values& data, Values& globals) override;
  bool updateValues(ExitDecision* decision, Values& status, Values& data, Values& globals) override;
  bool updateValues(ChoiceDecision* decision, Values& status, Values& data, Values& globals) override;
  bool updateValues(MessageDeliveryDecision* decision, Values& status, Values& data, Values& globals) override;

  std::shared_ptr<Evaluation> evaluate(EntryDecision* decision) override;
  std::shared_ptr<Evaluation> evaluate(ExitDecision* decision) override;
  std::shared_ptr<Evaluation> evaluate(ChoiceDecision* decision) override;
  std::shared_ptr<Evaluation> evaluate(MessageDeliveryDecision* decision) override;

  std::set<const BPMNOS::Model::Attribute*> getDependencies(EntryDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ExitDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ChoiceDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(MessageDeliveryDecision* decision) override;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GuidedEvaluator_H
