#ifndef BPMNOS_Execution_ExitDecision_H
#define BPMNOS_Execution_ExitDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token exiting a node.
 *
 * Transition from State::COMPLETION to State::DONE or State::DEPARTED
 */
struct ExitDecision : Decision {
  ExitDecision(const Token* token, std::optional<Values> exitStatus = std::nullopt);
  void processBy(Engine* engine) const override;
  std::optional<Values> exitStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ExitDecision_H

