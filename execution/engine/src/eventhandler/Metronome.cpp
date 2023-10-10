#include "Metronome.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include <thread>

using namespace BPMNOS::Execution;

Metronome::Metronome(unsigned int clockTickDuration)
{
  initialize(clockTickDuration);
}

void Metronome::initialize(unsigned int clockTickDuration) {
  this->clockTickDuration = clockTickDuration;
  timestamp = std::chrono::system_clock::now();
}

std::unique_ptr<Event> Metronome::fetchEvent( [[maybe_unused]] const SystemState& systemState ) {
  auto trigger = timestamp + std::chrono::milliseconds( clockTickDuration );
  std::this_thread::sleep_until(trigger);
  timestamp = std::chrono::system_clock::now();

  return std::make_unique<ClockTickEvent>();
}

