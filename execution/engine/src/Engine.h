#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include <set>
#include <vector>
#include "Event.h"
#include "EventHandler.h"
#include "StateMachine.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Engine {
public:
  Engine();
  std::vector<EventHandler*> eventHandlers;
  void addEventHandler(EventHandler* eventHandler);
  void start(BPMNOS::number clockTime = 0);
  std::unique_ptr<Event> listen( const SystemState& systemState );

//  void processEvent(Event* event);

/**
 * @brief Returns the timestamp the engine is in.
 */
  BPMNOS::number getCurrentTime();

/**
 * @brief Returns a reference to the system state
 */
  const SystemState& getSystemState();

protected:
  BPMNOS::number clockTick; ///< Timestep used to advance the current time by systemState.time += clockTick
  SystemState systemState;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Engine_H
