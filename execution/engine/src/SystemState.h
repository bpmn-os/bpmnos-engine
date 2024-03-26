#ifndef BPMNOS_Execution_SystemState_H
#define BPMNOS_Execution_SystemState_H

#include "StateMachine.h"
#include "DecisionRequest.h"
#include "execution/engine/src/Message.h"
#include "execution/utility/src/auto_list.h"
#include "execution/utility/src/auto_set.h"
#include "model/data/src/Scenario.h"
#include <set>
#include <queue>

namespace BPMNOS::Execution {

class Engine;


/**
 * @brief A class representing the state that the execution or simulation of a given scenario is in.
 */
class SystemState {
private:
  const Engine* engine;

public:
  SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime = 0);
  ~SystemState();

  /**
   * @brief Pointer to the corresponding scenario.
   */
  const BPMNOS::Model::Scenario* scenario;

  /**
   * @brief Timestamp holding the point in time that the engine is in (this is usually representing now).
   */
  BPMNOS::number currentTime;

  /**
   * @brief Timestamp holding the point in time that the simulation is in (this could be a future point in time).
   */
  std::optional<BPMNOS::number> assumedTime;

  /**
   * @brief Function returning the assumed time time if available or the current time otherwise.
   */
  BPMNOS::number getTime() const;

  /**
   * @brief Function returning true if there are tokens in the system or if there may be new instantiations of tokens.
   */
  bool isAlive() const;

  /**
   * @brief The total objective value (assuming maximization) accumulated during execution.
   *
   * Attributes declared for activities, event-subprocesses, and processes contribute to the objective when
   * - the activity is exited,
   * - a compensation activity is completed,
   * - the event-subprocess shuts down without failure, or
   * - the process shuts down without failure.
   *
   * In all other cases values of attributes declared to contribute to the objective will be ignored.
   */
  BPMNOS::number objective;
  
  auto_list< std::weak_ptr<Token>, std::shared_ptr<DecisionRequest> > pendingEntryEvents;
  auto_list< std::weak_ptr<Token>, std::shared_ptr<DecisionRequest> > pendingChoiceEvents;
  auto_list< std::weak_ptr<Token>, std::shared_ptr<DecisionRequest> > pendingExitEvents;
  auto_list< std::weak_ptr<Token>, std::shared_ptr<DecisionRequest> > pendingMessageDeliveryEvents;

  auto_list< std::weak_ptr<Token> > tokensAwaitingReadyEvent; ///< Container holding all tokens awaiting a ready event
  auto_set< BPMNOS::number, std::weak_ptr<Token> > tokensAwaitingCompletionEvent; ///< Sorted container holding all tokens awaiting a task completion event

  /**
   * @brief Container holding instance identifier and corresponding state machine pointer for each instantiation.
   */
  std::unordered_map< std::string, std::weak_ptr<StateMachine> > archive;

  /**
   * @brief Map holding unsent messages with recipient that isn't instantiated yet.
   */
  std::unordered_map<std::string, auto_list< std::weak_ptr<Message> > > unsent;

  /**
   * @brief Map holding the undelivered correspondence associated with a state machine which will be withdrawn when the state machine goes out of scope.
   */
  std::unordered_map<StateMachine*, auto_list< std::weak_ptr<Message> > > inbox;

  /**
   * @brief Map holding messages sent from given node.
   */
  std::unordered_map<const BPMN::FlowNode*, auto_list< std::weak_ptr<Message> > > outbox;

  //TODO: move below to StateMachine or Token?

  std::unordered_map< Token*, Token* > tokenAtSequentialPerformer; ///< Map holding a token at a sequential performer for each token awaiting sequential entry

  //TODO: make sure that elements are deleted when no longer required
  std::unordered_map< Token*, std::vector<Token*> > tokensAwaitingBoundaryEvent; ///< Map holding a container of all tokens at a boundary event awaiting to be triggered for each token at an activity
  std::unordered_map< Token*, Token* > tokenAssociatedToBoundaryEventToken; ///< Map holding the token residing at the associated activity for each token at a boundary event

  auto_set<BPMNOS::number, std::weak_ptr<Token>> tokensAwaitingTimer; ///< Sorted container holding holding all tokens awaiting a timer event

  std::unordered_map< Token*, std::weak_ptr<Message> > messageAwaitingDelivery; ///< Container holding message awaiting delivery for tokens at send tasks

  std::unordered_map< Token*, Token* > tokenAtMultiInstanceActivity; ///< Map holding the main token waiting at a multi-instance (or loop) activity.
  std::unordered_map< Token*, Token* > tokenAwaitingMultiInstanceExit; ///< Map holding the token waiting for the exit of an instantiation of a multi-instance (or loop) activity.
  std::unordered_map< Token*, std::vector<Token*> > tokensAtActivityInstance; ///< Map holding all tokens representing an active instance of a multi-instance (or loop) activity for each token waiting at the multi-instance (or loop) activity.
  std::unordered_map< Token*, std::vector<BPMNOS::Values> > exitStatusAtActivityInstance; ///< Map holding the exit status of all instances of a multi-instance (or loop) activity for each token waiting at the multi-instance (or loop) activity.

  std::unordered_map< Token*, Token* > tokenAtEventBasedGateway; ///< Map holding the token at the relevant event-based gateway
  std::unordered_map< Token*, std::vector<Token*> > tokensAwaitingEvent; ///< Map holding all tokens at catching events subsequent to an event-based gateway

  std::unordered_map< StateMachine*, std::map<const BPMN::FlowNode*, std::vector<Token*> > > tokensAwaitingGatewayActivation; ///< Map holding tokens awaiting activation of a converging gateway

//  std::unordered_map< StateMachine*, std::map<const BPMN::Node*, Token* > > tokensAwaitingCompensation; ///< Map holding tokens awaiting completion of a compensation

  std::unordered_map< Token*, Token* > tokenAwaitingCompensationActivity; ///< Map holding a token that waits for completion of another 
  std::unordered_map< StateMachine*, Token* > tokenAwaitingCompensationEventSubProcess; ///< Map holding a token that waits for completion of an event subprocess

  /**
   * @brief Container holding a state machine for each running instance.
   */
  StateMachines instances;

  /**
   * @brief Container holding all messages created by a throwing message event.
   */
  Messages messages;

  std::optional<BPMNOS::Values> getStatusAttributes(const StateMachine* root, const BPMN::Node* node) const;
  std::optional<BPMNOS::Values> getDataAttributes(const StateMachine* root, const BPMN::Node* node) const;

private:
  friend class Engine;
  friend class StateMachine;
  friend class Token;

  SystemState() = delete;

  /**
   * @brief Method returning a vector of all instantiations at the given time.
   */
  std::vector< std::tuple<const BPMN::Process*, BPMNOS::Values, BPMNOS::Values> > getInstantiations() const;

  void incrementTimeBy(BPMNOS::number duration);
  size_t instantiationCounter;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SystemState_H
