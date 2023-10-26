#ifndef BPMNOS_Execution_Engine_H
#define BPMNOS_Execution_Engine_H

#include <set>
#include <vector>
#include "Event.h"
#include "events/ClockTickEvent.h"
#include "events/TaskCompletionEvent.h"
#include "events/EntryEvent.h"
#include "events/ExitEvent.h"
//#include "events/MessageDeliveryEvent.h"
#include "events/ReadyEvent.h"
#include "events/TerminationEvent.h"
//#include "events/TimerEvent.h"
#include "EventHandler.h"
#include "StateMachine.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Listener;

class Engine {
public:
  Engine();
  void addEventHandler(EventHandler* eventHandler);
  void addListener(Listener* listener);

  std::unique_ptr<Event> fetchEvent();

  /**
   * @brief Runs a scenario as long as there is a token or new instantiations. Terminates when the time if the system exceeds the timeout.
   */
  void run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number timeout = std::numeric_limits<BPMNOS::number>::max());

  void process(const ClockTickEvent& event);
  void process(const TaskCompletionEvent& event);
  void process(const EntryEvent& event);
  void process(const ExitEvent& event);
//  void process(const MessageDeliveryEvent& event);
  void process(const ReadyEvent& event);
  void process(const TerminationEvent& event);
//  void process(const TimerEvent& event);

  /**
   * @brief Advance token as much as possible.
   */
  void advance(Token* token);

  /**
   * @brief Advance token as much as possible.
   */
  void advance(const Token* token);

/**
 * @brief Returns the timestamp the engine is in.
 */
  BPMNOS::number getCurrentTime();

/**
 * @brief Returns a pointer to the system state
 */
  const SystemState* getSystemState();

protected:

  BPMNOS::number clockTick; ///< Timestep used to advance the current time by systemState.time += clockTick
  std::unique_ptr<SystemState> systemState;
  bool advance();
  friend void Token::notify() const;
  std::vector<EventHandler*> eventHandlers;
  std::vector<Listener*> listeners;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
