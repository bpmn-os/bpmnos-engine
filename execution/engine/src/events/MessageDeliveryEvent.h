#ifndef BPMNOS_Execution_MessageDeliveryEvent_H
#define BPMNOS_Execution_MessageDeliveryEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"
#include "execution/engine/src/Message.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a message from the message pool being delivered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct MessageDeliveryEvent : Event {
  MessageDeliveryEvent(const Token* token, Messages::const_iterator message_it);
  void processBy(Engine* engine) const override;
  Messages::const_iterator message_it;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryEvent_H

