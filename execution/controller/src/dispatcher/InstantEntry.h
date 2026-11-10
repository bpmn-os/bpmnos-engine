#ifndef BPMNOS_Execution_InstantEntry_H
#define BPMNOS_Execution_InstantEntry_H

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Dispatches an entry event for the first token awaiting entry, without checking feasibility.
 *
 * Every token awaiting an entry decision is at a child of a sequential ad-hoc subprocess, the engine itself
 * entering any other activity as soon as its entry restrictions hold.
 */
class InstantEntry : public EventDispatcher {
public:
  InstantEntry();
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantEntry_H

