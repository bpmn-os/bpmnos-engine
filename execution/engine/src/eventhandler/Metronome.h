#ifndef BPMNOS_MetronomeHandler_H
#define BPMNOS_MetronomeHandler_H

#include <chrono>

#include <bpmn++.h>
#include "execution/engine/src/EventHandler.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a clock tick event each time fetchEvent() after sleeping in order to synchronize with real time.
 */
class Metronome : public EventHandler {
public:
  Metronome(unsigned int clockTickDuration = 1000);
  std::unique_ptr<Event> fetchEvent( const SystemState& systemState ) override;
  void initialize(unsigned int clockTickDuration); ///< Change clockTickDuration and set timestamp to current time and 
private:
  std::chrono::high_resolution_clock::time_point timestamp;
  unsigned int clockTickDuration; ///< Duration of clock ticks in milliseconds
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_MetronomeHandler_H

