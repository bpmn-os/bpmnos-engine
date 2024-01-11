#ifndef BPMNOS_Execution_SystemState_H
#define BPMNOS_Execution_SystemState_H

#include "StateMachine.h"
#include "execution/engine/src/Message.h"
#include "execution/utility/src/auto_list.h"
#include "execution/utility/src/auto_schedule.h"
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
   * @brief Container holding instance identifier and corresponding state machine pointer for each instantiation.
   */
  std::unordered_map< std::string, std::weak_ptr<StateMachine> > archive;

  /**
   * @brief Map holding unsent messages with recipient that isn't instantiated yet.
   */
  std::unordered_map<std::string, auto_list<Message> > unsent;

  /**
   * @brief Map holding the undelivered correspondence associated with a state machine which will be withdrawn when the state machine goes out of scope.
   */
  std::unordered_map<StateMachine*, auto_list<Message> > correspondence;

  /**
   * @brief Map holding messages sent from given node.
   */
  std::unordered_map<const BPMN::FlowNode*, auto_list<Message> > outbox;

  auto_list<Token> tokensAwaitingReadyEvent; ///< Container holding all tokens awaiting a ready event

  auto_list<Token> tokensAwaitingRegularEntryEvent; ///< Container holding all tokens at regular activities awaiting an entry event

  //TODO: make sure that elements are deleted when no longer required
  std::map< Token*, std::vector<Token*> > tokensAwaitingBoundaryEvent; ///< Map holding a container of all tokens at a boundary event awaiting to be triggered for each token at an activity
  std::unordered_map< Token*, Token* > tokenAtAssociatedActivity; ///< Map holding the token residing at the associated activity for each token at a boundary event

  //TODO: make sure that elements are deleted when no longer required
  std::unordered_map< Token*, auto_list<Token> > tokensAwaitingJobEntryEvent; ///< Map holding a container of all tokens awaiting entry at jobs for each token at an active resource

  auto_list<Token> tokensAtIdleResources; ///< Container holding indices of resources not executing a job and awaiting a job entry
//  std::vector< Token* > tokensAtActiveResources; ///< Container holding indices of tokens at busy resources

  auto_schedule<Token, Values> tokensAwaitingTaskCompletionEvent; ///< Sorted container holding all tokens awaiting a task completion event

  auto_list<Token> tokensAwaitingChoiceEvent; ///< Container holding all tokens awaiting a choice event

  auto_list<Token> tokensAwaitingResourceShutdownEvent; ///< Container holding all tokens awaiting a choice event

  auto_list<Token> tokensAwaitingExitEvent; ///< Container holding all tokens awaiting an exit event

  auto_schedule<Token> tokensAwaitingTimer; ///< Sorted container holding holding all tokens awaiting a timer event

  auto_list<Token,Values> tokensAwaitingMessageDelivery; ///< Container holding all tokens awaiting a message delivery event with associated header values

//  auto_list<Token> tokensAwaitingEventBasedGateway; ///< Container holding all tokens awaiting activation event for an event-based gateway

  //TODO: make sure that elements are deleted when no longer required
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

private:
  friend class Engine;
  friend class StateMachine;
  friend class Token;

  SystemState() = delete;

  /**
   * @brief Method returning a vector of all instantiations at the given time.
   */
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > getInstantiations() const;

  void incrementTimeBy(BPMNOS::number duration);
  size_t instantiationCounter;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SystemState_H
