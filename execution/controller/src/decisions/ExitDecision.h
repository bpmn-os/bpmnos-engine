#ifndef BPMNOS_Execution_ExitDecision_H
#define BPMNOS_Execution_ExitDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/ExitEvent.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token exiting a node.
 *
 * Transition from State::COMPLETION to State::DONE or State::DEPARTED
 */
struct ExitDecision : ExitEvent, Decision {
  ExitDecision(const Token* token, std::function<std::optional<double>(const Event* event)> evaluator = &Decision::nullEvaluator);

  std::optional<double> evaluate() override;

  static std::optional<double> localEvaluator(const Event* event);
  static std::optional<double> guidedEvaluator(const Event* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ExitDecision_H

