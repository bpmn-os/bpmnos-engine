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
  EntryDecision(const Token* token, Evaluator* evaluator);
  std::optional<double> evaluate() override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_EntryDecision_H

