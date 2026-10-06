#ifndef BPMNOS_Execution_Engine_H
#define BPMNOS_Execution_Engine_H

#include <set>
#include <vector>
#include <list>
#include <chrono>
#include "Event.h"
#include "events/TerminationEvent.h"
#include "events/ClockTickEvent.h"
#include "events/ErrorEvent.h"
#include "events/InstantiationEvent.h"
#include "events/SignalBroadcastEvent.h"
#include "events/ReadyEvent.h"
#include "events/EntryEvent.h"
#include "events/ChoiceEvent.h"
#include "events/CompletionEvent.h"
#include "events/MessageDeliveryEvent.h"
#include "events/ExitEvent.h"
//#include "Notifier.h"
#include "Mediator.h"
#include "EventDispatcher.h"
#include "Signal.h"
#include "SystemState.h"
#include "ConditionalEventObserver.h"
#include "Environment.h"

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
  static constexpr std::chrono::milliseconds SLEEP{1}; ///< Pause after a round yielding no event, bounding the rate at which the engine asks in vain

  /**
   * @brief Constructs an engine executing the given model for its whole lifetime.
   *
   * The engine shares ownership of the model and accepts only scenarios of it.
   */
  Engine(std::shared_ptr<const BPMNOS::Model::Model> model);

  ~Engine();
public:

  /**
   * @brief Runs a scenario of a data provider from the beginning, the engine taking ownership of it.
   *
   * Creates a fresh system state at the given start time and executes until a termination event is
   * processed. The start time must not be later than the earliest instantiation time, since an instance is
   * instantiated at the instant its instantiation time is reached and a later start would leave every
   * earlier instance uncreated.
   *
   * @param scenario The scenario to execute, created by a data provider built on the model of the engine
   * @param startTime Time the run begins at
   * @throws std::invalid_argument if the scenario is not one of the model of the engine
   */
  void run(std::unique_ptr<Scenario> scenario, BPMNOS::number startTime = 0);

  /**
   * @brief Initializes the engine with a fresh system state for a scenario of a data provider, the engine
   * taking ownership of it, and advances time to the run's first instant.
   *
   * Does not process any further event; call resume or advance afterwards. The opening clock tick is
   * announced like any other, so the record stream states the time the run begins at.
   *
   * @param scenario The scenario to execute, created by a data provider built on the model of the engine
   * @param startTime Time the run begins at; must not be later than the earliest instantiation time
   * @throws std::invalid_argument if the scenario is not one of the model of the engine
   */
  void initialize(std::unique_ptr<Scenario> scenario, BPMNOS::number startTime = 0);

  /**
   * @brief Initializes the engine's system state with a deep copy of a foreign system state and a scenario
   * of a data provider, the engine taking ownership of it.
   *
   * Does not run; call resume() afterwards to continue execution. The scenario is a fork or a new scenario
   * of the data provider of the run the foreign state belongs to.
   *
   * @param scenario The scenario to continue on, created by a data provider built on the model of the engine
   * @param foreignState The system state to copy
   * @throws std::invalid_argument if the scenario is not one of the model of the engine
   */
  void initializeSystemState(std::unique_ptr<Scenario> scenario, const SystemState* foreignState);

  /**
   * @brief Continues advancing the engine's existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   */
  void resume();

  /**
   * @brief Start processing the decision, then continues advancing the existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   *
   * @param decision The decision to process before greedy dispatch resumes
   */
  void resume(std::shared_ptr<Decision> decision);

  /**
   * @brief Start processing the event, then continues advancing the existing system state.
   *
   * Does not create a new state — run() or initializeSystemState() must have established one first.
   *
   * @param event The event to process before greedy dispatch resumes
   */
  void resume(std::shared_ptr<Event> event);

  /**
   * @brief Advance system state until next event has to be fetched.
   *
   * Performs one round, asking the environment for an event, then the dispatchers of the controller, and
   * notifying the environment if neither supplied one, and advances the system state by the event obtained
   * as far as possible without fetching the next event. If the round yields no event, it pauses for a moment
   * and returns without processing one.
   *
   * @return False once a termination event has been processed, and true otherwise.
   */
  bool advance();
private:
  /// @brief Calls advance until it returns false.
  void loop();
public:
  void process(const InstantiationEvent* event);
  void process(const SignalBroadcastEvent* event);
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
 * @brief Returns the model the engine executes, or nullptr if it has not yet been given a scenario.
 */
  const BPMNOS::Model::Model* getModel() const;

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

  /// @brief Method broadcasting a signal to every token awaiting it and instantiating the process it
  /// triggers, if any.
  ///
  /// The signal is announced before it is delivered anywhere, so that what is observed is the signal
  /// rather than what it causes. It is taken by value because it is broadcast from a command and the
  /// content it carries is fixed where the signal arises.
  ///
  /// Called for a signal thrown at a @ref BPMN::SignalThrowEvent "signal throw event" and for one the
  /// environment raises through a @ref SignalBroadcastEvent, so that a signal from outside reaches
  /// recipients exactly as one from inside.
  void broadcastSignal(Signal signal);

  /// @brief Method creating an instance of a process whenever the message or signal triggering it is
  /// thrown.
  ///
  /// The identifier is generated, an instance created by a trigger being declared nowhere, and the
  /// content of the trigger is applied to the initial status of the instance, which is where it is
  /// needed and after which it is of no further concern. The trigger being the condition for the start,
  /// the instance is started at once rather than upon a @ref ReadyEvent.
  void triggerInstance(const BPMN::Process* process, BPMNOS::VariedValueMap content);

  /// @brief Method creating an instance of a process from the message triggering it, and consuming that
  /// message.
  ///
  /// No token awaits a message at the @ref BPMN::MessageStartEvent "message start event" of a
  /// @ref BPMN::Process "process", the instance not existing before the message arrives, so no
  /// @ref MessageDeliveryRequest is created for it and no @ref MessageDeliveryEvent is ever dispatched.
  /// What such an event would do for the message and for the sender is therefore done here: the message
  /// is marked as delivered and withdrawn from the message pool, and a token at a
  /// @ref BPMN::SendTask "send task" awaiting its delivery is completed.
  void triggerInstanceByMessage(const BPMN::Process* process, std::weak_ptr<Message> message_ptr);

  void deleteInstance(StateMachine* instance); ///< Method removing completed instance

  /// @brief Method refusing a scenario of another model.
  void acceptScenario(const Scenario* scenario);

  const std::shared_ptr<const BPMNOS::Model::Model> model; ///< The model the engine executes, whose ownership it shares

  std::unique_ptr<SystemState> systemState;
  ConditionalEventObserver conditionalEventObserver;
  Environment environment;
  
//  friend void Token::notify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
