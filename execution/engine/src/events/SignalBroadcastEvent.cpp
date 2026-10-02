#include "SignalBroadcastEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

SignalBroadcastEvent::SignalBroadcastEvent(Signal signal)
  : Event(nullptr)
  , signal(std::move(signal))
{
}

void SignalBroadcastEvent::processBy(Engine* engine) const {
  engine->process(this);
}

bool SignalBroadcastEvent::expired() const {
  return false;
}

nlohmann::ordered_json SignalBroadcastEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "signal";
  // the signal is nested, so that the event is not mistaken for the signal it causes
  jsonObject["signal"] = signal.jsonify();

  return jsonObject;
}
