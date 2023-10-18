#ifndef BPMNOS_Execution_MessageDeliveryEvent_H
#define BPMNOS_Execution_MessageDeliveryEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/TriggerEvent.h"
#include "execution/engine/src/Message.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a message from the message pool being delivered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct MessageDeliveryEvent : TriggerEvent {
  MessageDeliveryEvent(const Token* token, std::optional<Values> status, std::optional< std::vector< std::unique_ptr<Message> >::const_iterator > message);
  void processBy(Engine* engine) const override;
  std::optional<Values> status;
  std::optional< std::vector< std::unique_ptr<Message> >::const_iterator > message;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryEvent_H

