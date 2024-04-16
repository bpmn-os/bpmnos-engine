#ifndef BPMNOS_Execution_GuidedEvaluator_H
#define BPMNOS_Execution_GuidedEvaluator_H

#include <bpmn++.h>
#include "LocalEvaluator.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending GuidedEvaluator
 */
class GuidedEvaluator : public LocalEvaluator {
public:
  bool updateValues(EntryDecision* decision, Values& status, Values& data) override;
  bool updateValues(ExitDecision* decision, Values& status, Values& data) override;
  bool updateValues(ChoiceDecision* decision, Values& status, Values& data) override;
  bool updateValues(MessageDeliveryDecision* decision, Values& status, Values& data) override;

  std::set<const BPMNOS::Model::Attribute*> getDependencies(EntryDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ExitDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(ChoiceDecision* decision) override;
  std::set<const BPMNOS::Model::Attribute*> getDependencies(MessageDeliveryDecision* decision) override;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_GuidedEvaluator_H
