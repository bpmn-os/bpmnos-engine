#ifndef BPMNOS_Execution_Message_H
#define BPMNOS_Execution_Message_H

#include <vector>
#include <string>
#include <memory>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

class Token;

class Message;
typedef std::vector< std::shared_ptr<Message> > Messages;

class Message : public std::enable_shared_from_this<Message> {
public:
  Message(Token* token, size_t index);
  const BPMN::FlowNode* origin;
  std::optional< std::string > recipient;
  BPMNOS::Values header;
  VariedValueMap contentValueMap;

  bool matches(const BPMNOS::Values& otherHeader); ///< Returns true if headers have the same size and all values that are defined are the same.

  void update(Token* token) const; ///< Updates the token status based on the message content.

  nlohmann::ordered_json jsonify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Message_H

