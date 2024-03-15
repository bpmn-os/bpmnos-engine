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
#include "events/EntryDecision.h"
#include "events/ChoiceDecision.h"
#include "events/CompletionEvent.h"
#include "events/MessageDeliveryDecision.h"
#include "events/ExitDecision.h"
//#include "Notifier.h"
#include "Mediator.h"
#include "EventDispatcher.h"
#include "SystemState.h"

namespace BPMNOS::Execution {

class Token;
class StateMachine;
//class Listener;
class Controller;

class Engine : public Mediator {
  friend class Token;
  friend class StateMachine;
//  friend void EventDispatcher::subscribe(Engine* engine);
public:
  Engine();
  ~Engine();
public:

  /**
   * @brief Runs a scenario as long as there is a token or new instantiations. Terminates when the time if the system exceeds the timeout.
   */
  void run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number timeout = std::numeric_limits<BPMNOS::number>::max());

  void process(const ReadyEvent* event);
  void process(const EntryDecision* event);
  void process(const ChoiceDecision* event);
  void process(const CompletionEvent* event);
  void process(const MessageDeliveryDecision* event);
  void process(const ExitDecision* event);
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
  std::unique_ptr<SystemState> systemState;
  bool advance();
//  friend void Token::notify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Engine_H
