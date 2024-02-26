#ifndef BPMNOS_Execution_Token_H
#define BPMNOS_Execution_Token_H

#include <bpmn++.h>
#include "model/parser/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cassert>

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
 * @ref BPMNOS::Model::Operator in the @ref BPMNOS::Model::ExtensionElements of the subprocess.
 * Furthermore, feasibility of the @ref status with respect to all relevant 
 * @ref BPMNOS::Model::Restriction in @ref BPMNOS::Model::ExtensionElements is checked.
 * If the status is infeasible, token transitions to the `FAILED` state and an error is raised
 * and handed over the the @ref StateMachine owning the token. Otherwise, a new token is generated at
 * the @ref BPMN::UntypedStartEvent of the subprocess and the token transitions to `BUSY` state.
 * @todo Describe token generation for boundary events and event subprocesses
 *
 * Upon entry to a @ref BPMN::Task, feasibility of the @ref status with respect to all relevant 
 * @ref BPMNOS::Model::Restriction in @ref BPMNOS::Model::ExtensionElements is checked.
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
 * @todo Desribe status updates conducted for each state. Operators for subprocesses must be instantaneous
 * and are applied before a token is created for the start event.
 * Operators for @ref BPMN::ReceiveTask and @ref BPMNOS::Model::DecisionTask must be instantaneous and are
 * applied after the message is received or the decision is made.
 * Operators for @ref BPMN::SendTask must be instantaneous and are applied before the message is sent.
 * Operators for all other tasks may be non-instantaneous. If they are instantaneous they are applied 
 * directly after entry. If they are non-instanteous, they are applied directly after entry, but the updated
 * state is only revealed upon completion of the task. When using the @ref DeterministicTaskCompletionHandler,
 * the updated status is revealed at the updated time attribute. Other task completion handler may override the
 * status values to represent non-deterministic cases, where the status changes of the opertaors only express
 * an expectation at the time the operators are applied.
 *
 * @todo Continue with documentation of state diagram.
 * - Token flow for @ref BPMN::Activity with @ref BPMN::Activity::isForCompensation is `true` is READY->WAITING->ENTERED->BUSY->COMPLETED. 
 * - Token flow for @ref BPMN::Activity with multi-instance marker is READY->WAITING->ENTERED->BUSY->COMPLETED->EXITED. 
 * - Token flow for @ref BPMN::EventBasedGateway is READY->BUSY->WAITING
 * All tokens can be WITHDRAWN at any time. 
 *
 * @note It is assumed that @ref XML::bpmn::tCompensateEventDefinition::waitForCompletion is `true`.
 *
 * @note It is assumed that @ref XML::bpmn::tAdHocSubProcess::ordering is `Sequential`.
 */
class Token : public std::enable_shared_from_this<Token> {
private:
  friend class SystemState;
  friend class StateMachine;
  friend class Engine;

public:
  const StateMachine* owner; ///< State machine owning the token
  std::shared_ptr<StateMachine> owned; ///< State machine owned by the token
  const BPMN::FlowNode* node;
  const BPMN::SequenceFlow* sequenceFlow;
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, EXITING, DEPARTED, ARRIVED, WAITING, DONE, FAILED, FAILING, WITHDRAWN }; ///< The states that a token can be in
  const BPMNOS::Model::AttributeMap& getAttributeMap() const;
private:
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "EXITING", "DEPARTED", "ARRIVED", "WAITING", "DONE", "FAILED", "FAILING", "WITHDRAWN" };
public:
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Token(const std::vector<Token*>& others);
  ~Token();

  State state;
  Values status;
  Token* performing; ///< Pointer to the activity token currently perfomed (only applies if node is a performer referenced by sequential ad-hoc subprocesses)
  
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

  bool isFeasible() const; ///< Check restrictions within current and ancestor scopes

  void advanceFromCreated();
  void advanceToReady();
  void advanceToEntered();
  void advanceToIdle();
  void advanceToBusy();

  void advanceToCompleted();

  void advanceToExiting();
  void advanceToDone();
  void advanceToDeparting();
  void advanceToDeparted(const BPMN::SequenceFlow* sequenceFlow);
  void advanceToArrived();

  void advanceToFailed();

  void terminate(); ///< Terminates state machine owned by token

  void awaitCompensation();

  void awaitReadyEvent(); ///< Wait for ready event for activities

  void awaitEntryEvent(); ///< Wait for entry event for activities

  void awaitChoiceEvent(); ///< Wait for choices to be made for decision tasks
  void awaitTaskCompletionEvent(); ///< Wait for completion event for tasks (except for decision tasks)
  void awaitExitEvent(); ///< Wait for exit event for activities

  void awaitTimer(BPMNOS::number time); ///< Wait for message trigger at catching events (except for catching message events)
  void awaitMessageDelivery(); ///< Wait for message delivery event

  void awaitEventBasedGateway(); ///< Wait for catching event at event-based gateways

  void awaitGatewayActivation(); ///< Wait for activiation of merging gateway

  void withdraw(); ///< Remove token

  void sendMessage( size_t index = 0 ); 

  Token* getSequentialPerfomerToken() const; ///< Returns token at sequential performer for tokens at activities within sequential adhoc subprocesses

  void update(State newState); ///< Updates token state and timestamp before calling notify()

  void notify() const; ///< Inform all listeners about token update

  /**
   * Returns a merged status from the status of each token
   **/
  template <typename TokenPtr>
  static BPMNOS::Values mergeStatus(const std::vector<TokenPtr>& tokens) {
    assert( !tokens.empty() );
    size_t n = tokens.front()->status.size();
    BPMNOS::Values result;
    result.resize(n);
    result[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp] = tokens.front()->status[(int)BPMNOS::Model::ExtensionElements::Index::Timestamp];

    for ( size_t i = 0; i < n; i++ ) {
      for ( auto& token : tokens ) {
        if ( i == (int)BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
          if ( result[i].value() < token->status[i].value() ) {
            result[i] = token->status[i];
          }
        }
        else if ( !result[i].has_value() ) {
          result[i] = token->status[i];
        }
        else if ( token->status[i].has_value() && token->status[i].value() != result[i].value() ) {
          result[i] = std::nullopt;
          break;
        }
      }
    }
    return result;
  }
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Token_H

