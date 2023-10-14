#ifndef BPMNOS_ReadyHandler_H
#define BPMNOS_ReadyHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a ready event when the required data is available a token at an activity.
 */
class ReadyHandler : public EventHandler {
  ReadyHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_ReadyHandler_H

