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
  Type getObservableType() const override { return type; };
  const Token* token;
  const Observable::Type type;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DecisionRequest_H
