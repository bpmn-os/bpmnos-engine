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
#include "events/MessageDeliveryEvent.h"
#include "events/ReadyEvent.h"
#include "events/TerminationEvent.h"
#include "events/TimerEvent.h"
#include "EventHandler.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Token;
class StateMachine;
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
  void process(const MessageDeliveryEvent& event);
  void process(const ReadyEvent& event);
  void process(const TerminationEvent& event);
  void process(const TimerEvent& event);

/**
 * @brief Returns the timestamp the engine is in.
 */
  BPMNOS::number getCurrentTime();

/**
 * @brief Returns a pointer to the system state
 */
  const SystemState* getSystemState();

protected:

  /**
   * @brief Class storing a command to be executed by the engine
   */
  class Command {
  public:
    Command(std::function<void()>&& f )
      : function(std::move(f)) {};

    Command(std::function<void()>&& f, StateMachine* stateMachine )
      : function(std::move(f))
      , stateMachine_ptr(stateMachine->weak_from_this()) {};

    Command(std::function<void()>&& f, Token* token )
      : function(std::move(f))
      , stateMachine_ptr(const_cast<StateMachine*>(token->owner)->weak_from_this())
      , token_ptr(token->weak_from_this()) {};

    void execute();
  private:
    std::function<void()> function;
    std::optional< std::weak_ptr<StateMachine> > stateMachine_ptr; ///< Pointer to the state machine that the command refers to
    std::optional< std::weak_ptr<Token> > token_ptr; ///< Pointer to the token that the command refers to
  };

  std::list<Command> commands; ///< List of commands to be executed

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
