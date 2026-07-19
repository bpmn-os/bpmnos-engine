#include "ClockTickEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ClockTickEvent::ClockTickEvent(const SystemState* systemState)
  : Event(nullptr)
  , time(systemState->getTime() + clockTick)
  , systemState(systemState)
{
}

void ClockTickEvent::processBy(Engine* engine) const {
  engine->process(this);
}

bool ClockTickEvent::expired() const {
  // The tick is stale if live time already reached its scheduled time; processing would otherwise
  // violate the strictly-monotonic time advance (SystemState::increaseTimeTo asserts time > currentTime).
  return time <= systemState->getTime();
}

nlohmann::ordered_json ClockTickEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "clocktick";
  jsonObject["time"] = (int)time;
  if ( expired() ) {
    jsonObject["expired"] = true;
  }

  return jsonObject;
}
