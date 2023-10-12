#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include <set>
#include <vector>
#include "Event.h"
#include "events/ChoiceEvent.h"
#include "events/ClockTickEvent.h"
#include "events/CompletionEvent.h"
#include "events/EntryEvent.h"
#include "events/ExitEvent.h"
#include "events/InstantiationEvent.h"
#include "events/MessageDeliveryEvent.h"
#include "events/ReadyEvent.h"
#include "events/TerminationEvent.h"
#include "events/TriggerEvent.h"
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

  void process(const ChoiceEvent& event);
  void process(const ClockTickEvent& event);
  void process(const CompletionEvent& event);
  void process(const EntryEvent& event);
  void process(const ExitEvent& event);
  void process(const InstantiationEvent& event);
  void process(const MessageDeliveryEvent& event);
  void process(const ReadyEvent& event);
  void process(const TerminationEvent& event);
  void process(const TriggerEvent& event);

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
