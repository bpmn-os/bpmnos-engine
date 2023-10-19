#ifndef BPMNOS_Execution_InstantEntryHandler_H
#define BPMNOS_Execution_InstantEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
class InstantEntryHandler : public EventHandler {
public:
  InstantEntryHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantEntryHandler_H

