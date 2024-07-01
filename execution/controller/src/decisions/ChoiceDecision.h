#ifndef BPMNOS_Execution_ChoiceDecision_H
#define BPMNOS_Execution_ChoiceDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

class Evaluator;

/**
 * @brief Represents the event that choices are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceDecision : ChoiceEvent, Decision {
  ChoiceDecision(const Token* token, Values choices, Evaluator* evaluator);
  std::optional<double> evaluate() override;
  
  nlohmann::ordered_json jsonify() const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceDecision_H
