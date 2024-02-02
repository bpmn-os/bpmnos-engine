#ifndef BPMNOS_Execution_NaiveSequentialEntryHandler_H
#define BPMNOS_Execution_NaiveSequentialEntryHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at an activity within a sequential adhoc subprocess.
 */
class NaiveSequentialEntryHandler : public EventHandler {
public:
  NaiveSequentialEntryHandler();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_NaiveSequentialEntryHandler_H

