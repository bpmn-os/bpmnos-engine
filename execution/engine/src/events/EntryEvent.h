#ifndef BPMNOS_EntryEvent_H
#define BPMNOS_EntryEvent_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a token entering a node.
 *
 * Transition from State::READY to State::ENTERED
 */
struct EntryEvent : Event {
  EntryEvent(Token* token, std::optional<Values> entryStatus = std::nullopt);
  void processBy(Engine* engine) const override;
  std::optional<Values> entryStatus;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_EntryEvent_H

