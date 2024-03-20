#ifndef BPMNOS_Execution_ChoiceDecision_H
#define BPMNOS_Execution_ChoiceDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event that choices are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceDecision : ChoiceEvent, Decision {
  ChoiceDecision(const Token* token, Values updatedStatus, std::function<std::optional<double>(Event* event)> evaluator = &Decision::nullEvaluator);
  Values updatedStatus;

  std::optional<double> evaluate() override;

  static std::optional<double> localEvaluator(Event* event);
  static std::optional<double> guidedEvaluator(Event* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceDecision_H
