#ifndef BPMNOS_Execution_Engine_H
#define BPMNOS_Execution_Engine_H

#include <set>
#include <vector>
#include <list> 
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
#include "Token.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Listener;

class Engine {
  friend class Token;
  friend class StateMachine;
public:
  Engine();
  ~Engine();
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
 * @brief Returns the timestamp the engine is in.
 */
  BPMNOS::number getCurrentTime();

/**
 * @brief Returns a pointer to the system state
 */
  const SystemState* getSystemState();

protected:
  class Command {
  public:
    template <typename Function, typename... Args>
    Command(StateMachine* stateMachine, Token* token, Function&& f, Args&&... args)
      : stateMachine(stateMachine)
      , token(token)
      , function(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)) {}
    void execute() { function(); }
    // Make the class movable
    Command(Command&& other) noexcept : function(std::move(other.function)) {}
    // Make the class non-copyable
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
  private:
    const StateMachine* stateMachine;
    const Token* token;
    std::function<void()> function;
  };

  std::list<Command> commands; ///< List of commands to be executed
  void clearCompletedStateMachines(); ///< Clears all completed state machines and advances parent tokens as much as possible

  void addInstances(); ///< Method adding all new instances and advancing tokens as much as possible

  void deleteInstance(StateMachine* instance); ///< Method removing completed instance

  BPMNOS::number clockTick; ///< Timestep used to advance the current time by systemState.time += clockTick
  std::unique_ptr<SystemState> systemState;
  bool advance();
//  friend void Token::notify() const;
  std::vector<EventHandler*> eventHandlers;
  std::vector<Listener*> listeners;

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
