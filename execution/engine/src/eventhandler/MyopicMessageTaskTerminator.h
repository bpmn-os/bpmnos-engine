#ifndef BPMNOS_Execution_MyopicMessageTaskTerminator_H
#define BPMNOS_Execution_MyopicMessageTaskTerminator_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an error event for a token awaiting delivery of a message when no other decision is pending.
 *
 * The MyopicMessageTaskTerminator terminates @ref BPMN:ReceiveTask and @ref BPMN:SendTask with an error if
 * all tokens have advanced as far as possible. Tokens are considered to have advanced as far as possible if 
 * all pending decisions except for message delivery decisions are made and if no task waits for completion and no
 * timer event waits to be triggered. It assumes that a message handler making message delivery decisions has been 
 * called before, so that no message can currently be delivered. The handler is myopic and does not consider
 * future instantiations of processes.
 */
class MyopicMessageTaskTerminator : public EventHandler {
public:
  MyopicMessageTaskTerminator();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MyopicMessageTaskTerminator_H

