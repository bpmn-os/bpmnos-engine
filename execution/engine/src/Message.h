#ifndef BPMNOS_Execution_Message_H
#define BPMNOS_Execution_Message_H

#include <vector>
#include <string>
#include <memory>
#include <bpmn++.h>
#include "Observable.h"
#include "model/utility/src/Number.h"
#include "model/bpmnos/src/extensionElements/AttributeRegistry.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

class Token;

class Message;
typedef std::vector< std::shared_ptr<Message> > Messages;

class Message : public Observable, public std::enable_shared_from_this<Message> {
public:
  constexpr Type getObservableType() const override { return Type::Message; };
  Message(Token* token, size_t index);
  enum class State { CREATED, DELIVERED, WITHDRAWN }; ///< The states that a message can be in
private:
  static inline std::string stateName[] = { "CREATED", "DELIVERED", "WITHDRAWN" };
public:
  State state;
  const BPMN::FlowNode* origin;
  Token* waitingToken;
  std::optional< BPMNOS::number > recipient;
  BPMNOS::Values header;
  VariedValueMap contentValueMap;

  bool matches(const BPMNOS::Values& otherHeader) const; ///< Returns true if headers have the same size and all values that are defined are the same.

  template <typename DataType>
  void apply(const BPMN::FlowNode* node, const BPMNOS::Model::AttributeRegistry& attributeRegistry, BPMNOS::Values& status, DataType& data) const; ///< Updates the status at a node based on the message content.

  nlohmann::ordered_json jsonify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Message_H

