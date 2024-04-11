#ifndef BPMNOS_Execution_Decision_H
#define BPMNOS_Execution_Decision_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include <functional>

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a pending decision
 */
class Decision : virtual public Event {
public:
  Decision(std::function<std::optional<double>(const Event* event)> evaluator = &Decision::nullEvaluator);

  constexpr static std::optional<double> nullEvaluator([[maybe_unused]] const Event* event) { return std::nullopt; }
  constexpr static std::optional<double> zeroEvaluator([[maybe_unused]] const Event* event) { return 0; }

  // entryRestrictions->dataInputs, exitRestrictions->dataInputs, operators->dataInputs, entryGuidance->dataInputs, choiceGuidance->dataInputs, messageDeliveryGuidance->dataInputs, exitGuidance->dataInputs
  // DataDependencies Entry; entryRestrictions->dataInputs + (Activity!Task ? operators->dataInputs)

  virtual std::optional<double> evaluate() = 0;
  std::optional<double> evaluation;
  
  bool timeDependent;
  std::vector<const BPMNOS::Model::Attribute*> dataDependencies;
protected:
  std::function< std::optional<double>(Event* event) > evaluator;
//  std::function< void(const BPMN::Node* node) > determineDependencies;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Decision_H
