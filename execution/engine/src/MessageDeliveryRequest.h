#ifndef BPMNOS_Execution_MessageDeliveryRequest_H
#define BPMNOS_Execution_MessageDeliveryRequest_H

#include "model/utility/src/Value.h"
#include "execution/engine/src/DecisionRequest.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents a pending message delivery, together with the header the awaiting token is matched by.
 *
 * The header is built once, from what the token holds when it comes to await a delivery, and is kept for as
 * long as the request is pending. That is the semantics of a delivery: a message is matched against what
 * the recipient presented when it began waiting, not against what it holds when a message happens to
 * arrive, so a data or global attribute written meanwhile does not change what this token accepts.
 *
 * Holding it here rather than rebuilding it means every candidate collector, every dispatcher and every
 * caller matches against the same header, and `MessageDefinition::getRecipientHeader` has one caller.
 */
struct MessageDeliveryRequest : DecisionRequest {
  MessageDeliveryRequest(const Token* token, BPMNOS::Values recipientHeader)
    : DecisionRequest(token, Observable::Type::MessageDeliveryRequest)
    , recipientHeader(std::move(recipientHeader))
  {};

  /// Returns what a decision request returns, and the header the token is matched by, each value rendered
  /// by the type its key carries.
  nlohmann::ordered_json jsonify() const override;

  const BPMNOS::Values recipientHeader; ///< Header the awaiting token is matched by.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MessageDeliveryRequest_H
