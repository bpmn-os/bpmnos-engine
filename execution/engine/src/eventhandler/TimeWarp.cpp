#include "TimeWarp.h"
#include "execution/engine/src/events/ClockTickEvent.h"

using namespace BPMNOS::Execution;

TimeWarp::TimeWarp()
{
}

std::unique_ptr<Event> TimeWarp::fetchEvent( [[maybe_unused]] const SystemState* systemState ) {
  return std::make_unique<ClockTickEvent>();
}

