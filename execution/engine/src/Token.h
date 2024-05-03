#ifndef BPMNOS_Execution_Token_H
#define BPMNOS_Execution_Token_H

#include <bpmn++.h>
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/Number.h"
#include "Observable.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cassert>

namespace BPMNOS::Execution {

class Engine;
class StateMachine;
class Token;
typedef std::vector< std::shared_ptr<Token> > Tokens;
class EventDispatcher;

/**
 * @brief Represents a token running through a (sub)process.
 * 
 * The `Token` class is used to represent tokens running through the execution flow of a (sub)process.
 * The execution logic is documented in section @ref execution_logic.
 *
 * @note All tokens can be WITHDRAWN at any time, e.g., when the respective state machine terminates.
 *
 * @note It is assumed that @ref XML::bpmn::tCompensateEventDefinition::waitForCompletion is `true`.
 *
 * @note It is assumed that @ref XML::bpmn::tAdHocSubProcess::ordering is `Sequential`.
 */
class Token : public Observable, public std::enable_shared_from_this<Token> {
private:
  friend class SystemState;
  friend class StateMachine;
  friend class Engine;

public:
  constexpr Type getObservableType() const override { return Type::Token; };
  const StateMachine* owner; ///< State machine owning the token
  std::shared_ptr<StateMachine> owned; ///< State machine owned by the token
  const BPMN::FlowNode* node;
  const BPMN::SequenceFlow* sequenceFlow;
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, EXITING, DEPARTED, ARRIVED, WAITING, DONE, FAILED, FAILING, WITHDRAWN }; ///< The states that a token can be in
  const BPMNOS::Model::AttributeRegistry& getAttributeRegistry() const;
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "EXITING", "DEPARTED", "ARRIVED", "WAITING", "DONE", "FAILED", "FAILING", "WITHDRAWN" };
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Token(const std::vector<Token*>& others);
  ~Token();

  State state;
  Values status;
  SharedValues* data; ///< Pointer to the data of the owner or owned state machine subprocesses)
  Values& globals;
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

  bool entryIsFeasible() const; ///< Check restrictions within current and ancestor scopes
  bool exitIsFeasible() const; ///< Check restrictions within current and ancestor scopes
//  bool satisfiesInheritedRestrictions() const; ///< Check restrictions within ancestor scopes

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

  template<typename DecisionType, typename... Args>
  std::shared_ptr<DecisionType> createDecisionRequest(Args&&... args);
    
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

