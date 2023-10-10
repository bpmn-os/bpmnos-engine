#ifndef BPMNOS_InstantEntryHandler_H
#define BPMNOS_InstantEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
struct InstantEntryHandler : EventHandler {
  InstantEntryHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_InstantEntryHandler_H

