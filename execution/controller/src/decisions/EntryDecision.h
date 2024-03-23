#ifndef BPMNOS_Execution_EntryDecision_H
#define BPMNOS_Execution_EntryDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token entering a node.
 *
 * Transition from State::READY to State::ENTERED
 */
struct EntryDecision : EntryEvent, Decision {
  EntryDecision(const Token* token, std::function<std::optional<double>(const Event* event)> evaluator = &Decision::nullEvaluator);

  std::optional<double> evaluate() override;

  static std::optional<double> localEvaluator(const Event* event);
  static std::optional<double> guidedEvaluator(const Event* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EntryDecision_H

