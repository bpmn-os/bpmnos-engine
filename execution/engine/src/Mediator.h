#ifndef BPMNOS_Execution_Mediator_H
#define BPMNOS_Execution_Mediator_H

#include "execution/engine/src/EventListener.h"
#include "execution/engine/src/Notifier.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an abstract base class for a class that is an event listener and notifier
 */
struct Mediator : EventListener, Notifier {
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Mediator_H
