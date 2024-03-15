#ifndef BPMNOS_Execution_ChoiceDecision_H
#define BPMNOS_Execution_ChoiceDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event that choices are made for a DecisionTask.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ChoiceDecision : Decision {
  constexpr Type getObservableType() const override { return Type::ChoiceRequest; };
  ChoiceDecision(const Token* token);
  void processBy(Engine* engine) const;
  Values updatedStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ChoiceDecision_H
