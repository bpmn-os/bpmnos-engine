#ifndef BPMNOS_Execution_MetronomeHandler_H
#define BPMNOS_Execution_MetronomeHandler_H

#include <chrono>

#include <bpmn++.h>
#include "execution/engine/src/EventDispatcher.h"

namespace BPMNOS::Execution {

/**
 * @brief Class creating a clock tick event each time fetchEvent() after sleeping in order to synchronize with real time.
 */
class Metronome : public EventDispatcher {
public:
  Metronome(unsigned int clockTickDuration = 1000);
  std::shared_ptr<Event> dispatchEvent( const SystemState* systemState ) override;
  void initialize(unsigned int clockTickDuration); ///< Change clockTickDuration and set timestamp to current time and 
private:
  std::chrono::high_resolution_clock::time_point timestamp;
  unsigned int clockTickDuration; ///< Duration of clock ticks in milliseconds
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_MetronomeHandler_H

