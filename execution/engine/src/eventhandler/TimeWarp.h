#ifndef BPMNOS_Execution_TimeWarpHandler_H
#define BPMNOS_Execution_TimeWarpHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a clock tick event each time fetchEvent() is called.
 */
class TimeWarp : public EventHandler {
public:
  TimeWarp();
  std::unique_ptr<Event> fetchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TimeWarpHandler_H

