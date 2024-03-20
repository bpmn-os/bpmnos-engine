#ifndef BPMNOS_Execution_EntryDecision_H
#define BPMNOS_Execution_EntryDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token entering a node.
 *
 * Transition from State::READY to State::ENTERED
 */
struct EntryDecision : Decision {
  EntryDecision(const Token* token, std::function<std::optional<double>(Decision* decision)> evaluator = &Decision::nullEvaluator);

  void processBy(Engine* engine) const override;
  std::optional<Values> entryStatus;

  static std::optional<double> localEvaluator(Decision* decision);
  static std::optional<double> guidedEvaluator(Decision* decision);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EntryDecision_H

