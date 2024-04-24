#ifndef BPMNOS_Execution_ReadyEvent_H
#define BPMNOS_Execution_ReadyEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token ready to enter a node inclduing necressary new attribute data.
 *
 * Transition from State::CREATED or State::ARRIVED to State::READY
 */
struct ReadyEvent : Event {
  ReadyEvent(const Token* token, BPMNOS::Values statusAttributes, BPMNOS::Values dataAttributes);
  void processBy(Engine* engine) const override;
  BPMNOS::Values statusAttributes;
  BPMNOS::Values dataAttributes;

  nlohmann::ordered_json jsonify() const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ReadyEvent_H

