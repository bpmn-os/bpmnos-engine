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
  virtual bool updateValues(EntryDecision* decision, Status& status, Data& data);
  virtual bool updateValues(ExitDecision* decision, Status& status, Data& data);
  virtual bool updateValues(ChoiceDecision* decision, Status& status, Data& data);
  virtual bool updateValues(MessageDeliveryDecision* decision, Status& status, Data& data);

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

#endif // BPMNOS_Execution_LocalEvaluator_H
