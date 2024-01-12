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

/**
 * @brief Represents a token running through the execution flow of a (sub)process.
 * 
 * The `Token` class is used to represent tokens running through the execution flow of a (sub)process. 
 * The following state diagram illustrates the life-cycle and state transitions of a token.
 * 
 * @image html Token_life-cycle.png
 *
 * At each @ref BPMN::UntypedStartEvent of a @ref BPMN::Process or @ref BPMN::SubProcess and at each
 * @ref BPMN::TypedStartEvent of an @ref BPMN::EventSubProcess a new token is initialized and
 * travels through the state diagram until a final state for the token is reached.
 * 
 * A token at an @ref BPMN::Activity can only transition to the `READY` state when a @ref ReadyEvent, 
 * that is e.g. generated by the @ref ReadyHandler, is processed by 
 * @ref Engine::process(const ReadyEvent& event). Tokens at other nodes do not transition to the
 * `READY` state and immeadiately transition to the `ENTERED` state.
 *
 * A token in `READY` state can only transition to the `ENTERED` state when an @ref EntryEvent, 
 * that is e.g. generated by the @ref InstantEntryHandler, is processed by 
 * @ref Engine::process(const EntryEvent& event).
 *
 * Upon entry to a @ref BPMN::SubProcess, the @ref status  is updated by applying all
 * @ref BPMNOS::Model::Operator in the @ref BPMNOS::Model::Status extension of the subprocess.
 * Furthermore, feasibility of the @ref status with respect to all relevant 
 * @ref BPMNOS::Model::Restriction in the @ref BPMNOS::Model::Status extension is checked.
 * If the status is infeasible, token transitions to the `FAILED` state and an error is raised
 * and handed over the the @ref StateMachine owning the token. Otherwise, a new token is generated at
 * the @ref BPMN::UntypedStartEvent of the subprocess and the token transitions to `BUSY` state.
 * @todo Describe token generation for boundary events and event subprocesses
 *
 * Upon entry to a @ref Task, feasibility of the @ref status with respect to all relevant 
 * @ref BPMNOS::Model::Restriction in the @ref BPMNOS::Model::Status extension is checked.
 * If the status is infeasible, token transitions to the `FAILED` state and an error is raised
 * and handed over the the @ref StateMachine owning the token. Otherwise, the token transitions 
 * to `BUSY` state.
 * @todo Describe token generation for boundary events
 *
 * For a @ref BPMN::ThrowEvent or a @ref BPMN::Gateway, the `BUSY`, `COMPLETED`, and
 * `EXITING` states are bypassed, and the token state immediately transitions to the next 
 * relevant state.
 * @attention The following throwing event types are not supported: @ref BPMN::SignalThrowEvent
 *
 * A token at a @ref BPMN::SubProcess remains in `BUSY` state as long as there is a remaining 
 * token within its scope. 
 *
 * A token at a @ref BPMN::Task remains in `BUSY` state until a @ref TaskCompletionEvent, 
 * that is e.g. generated by the @ref DeterministicTaskCompletionHandler, is processed by 
 * @ref Engine::process(const TaskCompletionEvent& event).
 * 
 * A token at a @ref BPMN::MessageCatchEvent remains in `BUSY` state until a 
 * @ref MessageDeliveryEvent, that is e.g. generated by the @ref FirstMatchingMessageHandler,
 * is processed by @ref Engine::process(const MessageDeliveryEvent& event).
 *
 * @attention The following catching event types are not supported: @ref BPMN::ConditionalCatchEvent,
 * @ref BPMN::SignalCatchEvent
 *
 * @todo Add support for @ref BPMN::TimerCatchEvent
 * 
 * @todo Continue with documentation of state diagram
 *
 * @note It is assumed that @ref XML::bpmn::tCompensateEventDefinition::waitForCompletion is `true`.
 */
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
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, EXITING, DEPARTED, ARRIVED, WAITING, DONE, FAILED, FAILING, WITHDRAWN }; ///< The states that a token can be in
  const BPMNOS::Model::AttributeMap& getAttributeMap() const;
private:
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "EXITING", "DEPARTED", "ARRIVED", "WAITING", "DONE", "FAILED", "FAILING", "WITHDRAWN" };
  State state;
public:
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Token(const std::vector<Token*>& others);
  ~Token();

  Values status;
  void setStatus(const BPMNOS::Values& other); ///< Copies all elements except the instance id from `other` to `status`

  bool ready() const { return state == State::READY; };
  bool entered() const { return state == State::ENTERED; };
  bool busy() const { return state == State::BUSY; };
  bool completed() const { return state == State::COMPLETED; };
  bool arrived() const { return state == State::ARRIVED; };
  bool waiting() const { return state == State::WAITING; };
  bool done() const { return state == State::DONE; };
  bool failed() const { return state == State::FAILED; };

  nlohmann::ordered_json jsonify() const;
private:

  bool isFeasible(); ///< Check restrictions within current and ancestor scopes

//  void advanceFromCreated();
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

  void awaitGatewayActivation(); ///< Wait for activiation of merging gateway

  void withdraw(); ///< Remove token from all data

  Token* getResourceToken() const; ///< Returns token at resource activity for tokens at jobs and nullptr for all other tokens

  void update(State newState); ///< Updates token state and timestamp before calling notify()

  void notify() const; ///< Inform all listeners about token update

  void mergeStatus(const Token* other); ///< Merges the status of the other token into the status

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Token_H

