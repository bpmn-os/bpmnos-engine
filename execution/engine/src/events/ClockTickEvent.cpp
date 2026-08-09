#include "ClockTickEvent.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <stdexcept>

using namespace BPMNOS::Execution;

ClockTickEvent::ClockTickEvent(const SystemState* systemState)
  : Event(nullptr)
  , time(systemState->getTime() + clockTick)
  , systemState(systemState)
{
}

ClockTickEvent::ClockTickEvent(const SystemState* systemState, BPMNOS::number time)
  : Event(nullptr)
  , time(time)
  , systemState(systemState)
{
  if ( systemState->getTime() != std::numeric_limits<BPMNOS::number>::lowest() ) {
    // during a run the clock advances one clockTick at a time: an instance is instantiated at the instant
    // its instantiation time is reached, so a tick that skipped an instant would drop it silently. A state
    // still standing at the lowest representable time is one whose run has not started.
    throw std::logic_error("ClockTickEvent: time may only be given before the run has started");
  }
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
