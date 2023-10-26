#ifndef BPMNOS_Execution_ResourceShutdownEvent_H
#define BPMNOS_Execution_ResourceShutdownEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/events/CompletionEvent.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a shutdown of an idle resource.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct ResourceShutdownEvent : CompletionEvent {
  ResourceShutdownEvent(const Token* token, const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >& choices);
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_ResourceShutdownEvent_H

