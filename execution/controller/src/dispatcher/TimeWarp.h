#ifndef BPMNOS_Execution_TimeWarpHandler_H
#define BPMNOS_Execution_TimeWarpHandler_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a clock tick event each time fetchEvent() is called.
 */
class TimeWarp : public EventDispatcher {
public:
  TimeWarp();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TimeWarpHandler_H

