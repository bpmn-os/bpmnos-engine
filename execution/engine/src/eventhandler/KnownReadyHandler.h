#ifndef BPMNOS_KnownReadyHandler_H
#define BPMNOS_KnownReadyHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a ready event when the required data is available a token at an activity.
 */
struct KnownReadyHandler : EventHandler {
  KnownReadyHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_KnownReadyHandler_H

