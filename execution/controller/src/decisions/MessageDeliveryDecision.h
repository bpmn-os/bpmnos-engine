#ifndef BPMNOS_Execution_MessageDeliveryDecision_H
#define BPMNOS_Execution_MessageDeliveryDecision_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Message.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "execution/controller/src/Decision.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a message from the message pool being delivered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct MessageDeliveryDecision : MessageDeliveryEvent, Decision {
  MessageDeliveryDecision(const Token* token, const Message* message, std::function<std::optional<double>(Event* event)> evaluator = &Decision::nullEvaluator);

  std::optional<double> evaluate() override;

  static std::optional<double> localEvaluator(Event* event);
  static std::optional<double> guidedEvaluator(Event* event);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryDecision_H

