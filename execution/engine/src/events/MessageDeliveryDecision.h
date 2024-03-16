#ifndef BPMNOS_Execution_MessageDeliveryDecision_H
#define BPMNOS_Execution_MessageDeliveryDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Message.h"
#include "execution/engine/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a message from the message pool being delivered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct MessageDeliveryDecision : Decision {
  MessageDeliveryDecision(const Token* token, Message* message, const BPMNOS::Values recipientHeader );
  void processBy(Engine* engine) const override;
  const Message* message;
  const BPMNOS::Values recipientHeader;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryDecision_H

