#ifndef BPMNOS_Execution_InstantiationEvent_H
#define BPMNOS_Execution_InstantiationEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a process instance becoming known.
 *
 * Processing the event creates the instance with its token at the process in State::CREATED, which awaits
 * the ready event starting the instance. The event carries the status and data of the instance as far as
 * they are disclosed when the instance becomes known.
 */
struct InstantiationEvent : Event {
  InstantiationEvent(const BPMN::Process* process, BPMNOS::Values status, BPMNOS::Values data);
  void processBy(Engine* engine) const override;

  /// Never stale, since the event refers to no token.
  bool expired() const override;

  nlohmann::ordered_json jsonify() const override;

  const BPMN::Process* process;
  BPMNOS::Values status;
  BPMNOS::Values data;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstantiationEvent_H
