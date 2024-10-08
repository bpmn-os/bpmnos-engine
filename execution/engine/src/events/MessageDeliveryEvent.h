#ifndef BPMNOS_Execution_MessageDeliveryEvent_H
#define BPMNOS_Execution_MessageDeliveryEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Message.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a message from the message pool being delivered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct MessageDeliveryEvent : virtual Event {
  MessageDeliveryEvent(const Token* token, const Message* message);
  void processBy(Engine* engine) const override;
  std::weak_ptr<const Message> message;

  nlohmann::ordered_json jsonify() const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryEvent_H

