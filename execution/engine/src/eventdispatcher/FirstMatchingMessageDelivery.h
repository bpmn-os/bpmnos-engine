#ifndef BPMNOS_Execution_FirstMatchingMessageDelivery_H
#define BPMNOS_Execution_FirstMatchingMessageDelivery_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating an entry event for a token awaiting the entry at a regular activity (i.e. not a job).
 */
class FirstMatchingMessageDelivery : public EventDispatcher {
public:
  FirstMatchingMessageDelivery();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_FirstMatchingMessageDelivery_H

