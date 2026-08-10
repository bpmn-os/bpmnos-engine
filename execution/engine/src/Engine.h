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
class Decision;

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
   * Creates a fresh system state at the given start time and executes until no tokens remain,
   * no new instantiations are pending, or the end time is exceeded. The start time must not be later
   * than the scenario's earliest instantiation time, since an instance is instantiated at the instant
   * its instantiation time is reached and a later start would leave every earlier instance uncreated.
   *
   * @param scenario The scenario to execute
   * @param startTime Time the run begins at
   * @param endTime Last time to process (engine stops when time >= endTime)
   */
  void run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number startTime = 0, BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Initializes the engine with a fresh system state and advances time to the run's first instant.
   *
   * Does not process any further event; call run, resume, or step afterwards. The opening clock tick is
   * announced like any other, so the record stream states the time the run begins at and deferred data is
   * disclosed for that instant.
   *
   * @param scenario The scenario to execute
   * @param startTime Time the run begins at; must not be later than the scenario's earliest instantiation
   *        time, since an instance is instantiated at the instant its instantiation time is reached and a
   *        later start would leave every earlier instance uncreated
   */
  void initialize(const BPMNOS::Model::Scenario* scenario, BPMNOS::number startTime = 0);

  /**
   * @brief Initializes the engine's system state with a deep copy of a foreign system state.
   *
   * Does not run; call resume() afterwards to continue execution. The copy already holds every instance
   * due up to its current time, so the instantiation watermark starts at that time.
   *
   * @param scenario The scenario to use (may be forked from the original)
   * @param foreignState The system state to copy
   */
  void initializeSystemState(const BPMNOS::Model::Scenario* scenario, const SystemState* foreignState);

  /**
   * @brief Continues advancing the engine's existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   *
   * @param endTime Last time to process (engine stops when time >= endTime)
   */
  void resume(BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Start processing the decision, then continues advancing the existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   *
   * @param decision The decision to process before greedy dispatch resumes
   * @param endTime Last time to process (engine stops when time >= endTime)
   */
  void resume(std::shared_ptr<Decision> decision, BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Start processing the event, then continues advancing the existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   *
   * @param event The event to process before greedy dispatch resumes
   * @param endTime Last time to process (engine stops when time >= endTime)
   */
  void resume(std::shared_ptr<Event> event, BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());

  /**
   * @brief Advance system state until next event has to be fetched.
   *
   * Fetches a single event and advances the system state as far as possible without fetching the next event.
   *
   * @param endTime Last time to process (the engine stops before a clock tick beyond it)
   * @return True if an event was processed and the run may continue.
   */
  bool advance(BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());
private:
  void run(BPMNOS::number endTime = std::numeric_limits<BPMNOS::number>::max());
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
  BPMNOS::number getCurrentTime() const;

/**
 * @brief Returns a pointer to the system state
 */
  const SystemState* getSystemState() const;

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

  void processCommands(); ///< Method executing all enqueued commands, including those enqueued by a command being executed

  void addInstances(); ///< Method adding all new instances and advancing tokens as much as possible

  void deleteInstance(StateMachine* instance); ///< Method removing completed instance

  BPMNOS::number lastInstantiationTime; ///< Timestamp when instances were last added (to prevent duplicate additions at same time)
  std::unique_ptr<SystemState> systemState;
  ConditionalEventObserver conditionalEventObserver;
  ScenarioUpdater scenarioUpdater;
  ReadyHandler readyHandler;
  TaskCompletionHandler taskCompletionHandler;
  
//  friend void Token::notify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
