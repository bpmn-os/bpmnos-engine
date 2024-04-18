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
  ExitDecision(const Token* token, Evaluator* evaluator);
  std::optional<double> evaluate() override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ExitDecision_H

