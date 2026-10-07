#ifndef BPMNOS_Execution_SignalBroadcastEvent_H
#define BPMNOS_Execution_SignalBroadcastEvent_H

#include <bpmn++.h>
#include "execution/engine/src/Event.h"
#include "execution/engine/src/Signal.h"

namespace BPMNOS::Execution {

/**
 * @brief Represents the event of a signal raised by the environment.
 *
 * Processing the event broadcasts the signal exactly as a signal thrown within the model is broadcast.
 * The event is dispatched by the @ref Environment for the signals a data provider enqueues, whereas a
 * signal thrown within the model is broadcast without an event.
 */
struct SignalBroadcastEvent : Event {
  SignalBroadcastEvent(Signal signal);
  void processBy(Engine* engine) const override;

  /// Never stale, since the event refers to no token.
  bool expired() const override;

  nlohmann::ordered_json jsonify() const override;

  Signal signal;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SignalBroadcastEvent_H
