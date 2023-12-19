#ifndef BPMNOS_Execution_Message_H
#define BPMNOS_Execution_Message_H

#include <vector>
#include <string>
#include <memory>
#include <bpmn++.h>
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

class Token;

class Message;
typedef std::vector< std::unique_ptr<Message> > Messages;

class Message {
public:
  Message(Token* token);
  const BPMN::FlowNode* origin;
  BPMNOS::Values header;
  VariedValueMap contentValueMap;

  bool matches(const BPMNOS::Values& otherHeader); ///< Returns true if headers have the same size and all values that are defined are the same.

  void update(Token* token) const; ///< Updates the token status based on the message content.
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Message_H

