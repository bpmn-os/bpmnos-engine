#ifndef BPMNOS_TriggerEvent_H
#define BPMNOS_TriggerEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a catching BPMN event being triggered.
 *
 * Transition from State::BUSY to State::COMPLETED
 */
struct TriggerEvent : Event {
  TriggerEvent(Token* token);
  void processBy(Engine* engine) const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_TriggerEvent_H

