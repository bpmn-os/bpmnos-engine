#ifndef BPMNOS_Execution_InstantExit_H
#define BPMNOS_Execution_InstantExit_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an exit event for a token awaiting the exit at an activity.
 */
class InstantExit : public EventDispatcher {
public:
  InstantExit();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantExit_H

