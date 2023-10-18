#ifndef BPMNOS_Execution_Message_H
#define BPMNOS_Execution_Message_H

#include <vector>
#include <string>
#include <memory>

namespace BPMNOS::Execution {

struct Message;
typedef std::vector< std::unique_ptr<Message> > Messages;

struct Message {
  std::string name;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Message_H

