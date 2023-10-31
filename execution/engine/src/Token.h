#ifndef BPMNOS_Execution_Token_H
#define BPMNOS_Execution_Token_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

class StateMachine;
class Token;
typedef std::vector< Token > Tokens;

class Token {
private:
  friend class SystemState;
  friend class StateMachine;
  friend class Engine;

public:
  const StateMachine* owner;
  const BPMN::FlowNode* node; 
private:
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, EXITING, ARRIVED, DONE, FAILED };
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "EXITING", "ARRIVED", "DONE", "FAILED" };
  State state;
public:
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Values status;

  bool ready() const { return state == State::READY; };
  bool entered() const { return state == State::ENTERED; };
  bool busy() const { return state == State::BUSY; };
  bool completed() const { return state == State::COMPLETED; };
  bool exiting() const { return state == State::EXITING; };
  bool arrived() const { return state == State::ARRIVED; };
  bool done() const { return state == State::DONE; };
  bool failed() const { return state == State::FAILED; };

  nlohmann::json jsonify() const;
private:

  bool isFeasible(); ///< Check restrictions within current and ancestor scopes

  void advanceFromCreated();
  void advanceToReady(std::optional< std::reference_wrapper<const Values> > values = std::nullopt );
  void advanceToEntered(std::optional< std::reference_wrapper<const Values> > statusUpdate = std::nullopt );
  void advanceToBusy();
  void advanceToCompleted(const std::vector< std::pair< size_t, std::optional<BPMNOS::number> > > updatedValues = std::vector< std::pair< size_t, std::optional<BPMNOS::number> > >() );
  void advanceToExiting(std::optional< std::reference_wrapper<const Values> > statusUpdate = std::nullopt );
  void advanceToDone();
  void advanceToDeparting();
  void advanceToArrived(const BPMN::FlowNode* destination);

  void advanceToFailed();

  void awaitReadyEvent(); ///< Wait for ready event for activities

  void awaitEntryEvent(); ///< Wait for entry event for activities

  void awaitChoiceEvent(); ///< Wait for choices to be made for decision tasks
  void awaitTaskCompletionEvent(); ///< Wait for completion event for tasks (except for decision tasks)
  void awaitResourceShutdownEvent(); ///< Wait for completion event for tasks (except for decision tasks)
  void awaitExitEvent(); ///< Wait for exit event for activities

  void awaitTimer(BPMNOS::number time); ///< Wait for message trigger at catching events (except for catching message events)
  void awaitMessageDelivery(); ///< Wait for message delivery event

  void awaitEventBasedGateway(); ///< Wait for catching event at event-based gateways 

  void awaitStateMachineCompletion(); ///< Wait for completion of all tokens within a scope
  void awaitDisposal(); ///< Wait for disposal of token to occur when all tokens are done or token flow of parent failed or is interrupted

  void awaitGatewayActivation(); ///< Wait for activiation of merging gateway

  Token* getResourceToken(); ///< Returns token at resource activity for tokens at jobs and nullptr for all other tokens

  void update(State newState); ///< Updates token state and timestamp before calling notify()

  void notify() const; ///< Inform all listeners about token update

 

/*
  template <typename F, typename... Args>
    void update(State newState, F&& updateStatus, Args... args) {
    updateStatus(this, std::forward<Args>(args)...);
    update(newState);
  }
*/
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Token_H

