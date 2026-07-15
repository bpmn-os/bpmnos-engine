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

nlohmann::ordered_json ClockTickEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "clocktick";
  jsonObject["time"] = (int)time;

  return jsonObject;
}
