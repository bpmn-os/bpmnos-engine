#include "TimeWarp.h"
#include "execution/engine/src/events/ClockTickEvent.h"

using namespace BPMNOS::Execution;

TimeWarp::TimeWarp()
{
}

std::shared_ptr<Event> TimeWarp::dispatchEvent( const SystemState* systemState ) {
  return std::make_shared<ClockTickEvent>(systemState);
}

