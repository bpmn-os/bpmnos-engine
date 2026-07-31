#ifndef BPMNOS_Execution_DecisionRequest_H
#define BPMNOS_Execution_DecisionRequest_H

#include <bpmn++.h>
#include "execution/engine/src/Token.h"
#include "execution/engine/src/Observable.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents a pending decision
 */
struct DecisionRequest : Observable, public std::enable_shared_from_this<DecisionRequest>  {
  DecisionRequest(const Token* token, const Observable::Type type) : token(token), type(type) {};
  virtual ~DecisionRequest() = default;
  Type getObservableType() const override { return type; };

  /// Returns the kind of decision awaited and the token awaiting it. A request holding terms of its own
  /// adds them.
  virtual nlohmann::ordered_json jsonify() const {
    nlohmann::ordered_json jsonObject;
    jsonObject["type"] = typeName();
    jsonObject["token"] = token->jsonify();
    return jsonObject;
  };

  const Token* token;
  const Observable::Type type;

protected:
  std::string typeName() const {
    switch ( type ) {
      case Observable::Type::EntryRequest: return "entry";
      case Observable::Type::ChoiceRequest: return "choice";
      case Observable::Type::ExitRequest: return "exit";
      case Observable::Type::MessageDeliveryRequest: return "messageDelivery";
      default: break;
    }
    assert(false && "Unexpected type of decision request");
    return "";
  };
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DecisionRequest_H
