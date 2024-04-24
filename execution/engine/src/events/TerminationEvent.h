#ifndef BPMNOS_Execution_TerminationEvent_H
#define BPMNOS_Execution_TerminationEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents an event causing the engine to terminate.
 */
struct TerminationEvent : Event {
  TerminationEvent();
  void processBy(Engine* engine) const override;

  nlohmann::ordered_json jsonify() const override;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_TerminationEvent_H

