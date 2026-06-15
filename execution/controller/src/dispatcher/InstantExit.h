#ifndef BPMNOS_Execution_InstantExit_H
#define BPMNOS_Execution_InstantExit_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Dispatches an exit event for the first token awaiting exit, without checking feasibility.
 */
class InstantExit : public EventDispatcher {
public:
  InstantExit();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantExit_H

