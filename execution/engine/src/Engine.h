#ifndef BPMNOS_Execution_Engine_H
#define BPMNOS_Execution_Engine_H

#include <set>
#include <vector>
#include <list>
#include "Event.h"
#include "events/TerminationEvent.h"
#include "events/ClockTickEvent.h"
#include "events/ErrorEvent.h"
#include "events/ReadyEvent.h"
#include "events/EntryEvent.h"
#include "events/ChoiceEvent.h"
#include "events/CompletionEvent.h"
#include "events/MessageDeliveryEvent.h"
#include "events/ExitEvent.h"
//#include "Notifier.h"
#include "Mediator.h"
#include "EventDispatcher.h"
#include "SystemState.h"
#include "ConditionalEventObserver.h"
#include "ScenarioUpdater.h"
#include "ReadyHandler.h"
#include "TaskCompletionHandler.h"

namespace BPMNOS::Execution {

class Token;
class StateMachine;
//class Listener;
class Controller;

class Engine : public Mediator {
  friend class Token;
  friend class StateMachine;
  friend class ConditionalEventObserver;
//  friend void EventDispatcher::subscribe(Engine* engine);
public:
  Engine();
  ~Engine();
public:

  /**
   * @brief Runs a scenario from the beginning.
   *
   * Creates a fresh system state and executes until no tokens remain,
   * no new instantiations are pending, or the timeout is exceeded.
   *
   * @param scenario The scenario to execute
   * @param timeout Last time to process (engine stops when time >= timeout)
   * @return The objective value at termination
   */
  BPMNOS::number run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number timeout = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Resumes execution from a given system state.
   *
   * Creates a copy of the foreign state and continues execution for the given scenario.
   *
   * @param scenario The scenario to use (may be forked from original)
   * @param foreignState The system state to resume from
   * @param timeout Last time to process (engine stops when time >= timeout)
   * @return The objective value at termination
   */
  BPMNOS::number resume(const BPMNOS::Model::Scenario* scenario, const SystemState* foreignState, BPMNOS::number timeout = std::numeric_limits<BPMNOS::number>::max());
private:
  BPMNOS::number run(BPMNOS::number timeout = std::numeric_limits<BPMNOS::number>::max());
public:
  void process(const ReadyEvent* event);
  void process(const EntryEvent* event);
  void process(const ChoiceEvent* event);
  void process(const CompletionEvent* event);
  void process(const MessageDeliveryEvent* event);
  void process(const ExitEvent* event);
  void process(const ErrorEvent* event);
  void process([[maybe_unused]] const ClockTickEvent* event);
  void process([[maybe_unused]] const TerminationEvent* event);

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
  BPMNOS::number lastInstantiationTime; ///< Timestamp when instances were last added (to prevent duplicate additions at same time)
  std::unique_ptr<SystemState> systemState;
  ConditionalEventObserver conditionalEventObserver;
  ScenarioUpdater scenarioUpdater;
  ReadyHandler readyHandler;
  TaskCompletionHandler taskCompletionHandler;
  
  bool advance(BPMNOS::number timeout);
  bool terminated;
//  friend void Token::notify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
