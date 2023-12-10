#ifndef BPMNOS_Execution_Token_H
#define BPMNOS_Execution_Token_H

#include <bpmn++.h>
#include "model/parser/src/extensionElements/Status.h"
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace BPMNOS::Execution {

class Engine;
class StateMachine;
class Token;
typedef std::vector< std::shared_ptr<Token> > Tokens;

class Token : public std::enable_shared_from_this<Token> {
private:
  friend class SystemState;
  friend class StateMachine;
  friend class Engine;

public:
  const StateMachine* owner; ///< State machine owning the token
  StateMachine* owned; ///< State machine owned by the token
  const BPMN::FlowNode* node; 
  const BPMN::SequenceFlow* sequenceFlow; 
private:
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, EXITING, DEPARTED, ARRIVED, WAITING, DONE, FAILED };
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "EXITING", "DEPARTED", "ARRIVED", "WAITING", "DONE", "FAILED" };
  State state;
public:
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Token(const std::vector<Token*>& others);
  ~Token();

  Values status;

  bool ready() const { return state == State::READY; };
  bool entered() const { return state == State::ENTERED; };
  bool busy() const { return state == State::BUSY; };
  bool completed() const { return state == State::COMPLETED; };
  bool exiting() const { return state == State::EXITING; };
  bool arrived() const { return state == State::ARRIVED; };
  bool waiting() const { return state == State::WAITING; };
  bool done() const { return state == State::DONE; };
  bool failed() const { return state == State::FAILED; };

  nlohmann::ordered_json jsonify() const;
private:
  const BPMNOS::Model::AttributeMap& getAttributeMap() const;

  bool isFeasible(); ///< Check restrictions within current and ancestor scopes

  void advanceFromCreated();
  void advanceToReady();
  void advanceToEntered();
  void advanceToBusy();

  void advanceToCompleted();

  void advanceToExiting();
  void advanceToDone();
  void advanceToDeparting();
  void advanceToDeparted(const BPMN::SequenceFlow* sequenceFlow);
  void advanceToArrived();

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

  void awaitGatewayActivation(); ///< Wait for activiation of merging gateway

  void destroy(); ///< Remove token from all data

  Token* getResourceToken() const; ///< Returns token at resource activity for tokens at jobs and nullptr for all other tokens

  void update(State newState); ///< Updates token state and timestamp before calling notify()

  void notify() const; ///< Inform all listeners about token update

  void mergeStatus(const Token* other); ///< Merges the status of the other token into the status

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Token_H

