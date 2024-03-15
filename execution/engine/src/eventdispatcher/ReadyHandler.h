#ifndef BPMNOS_Execution_ReadyHandler_H
#define BPMNOS_Execution_ReadyHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class dispatching a ready event when the required data is available a token at an activity.
 */
class ReadyHandler : public EventDispatcher {
public:
  ReadyHandler();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ReadyHandler_H

