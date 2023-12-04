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

  struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
  };

public:
  SystemState(const Engine* engine, const BPMNOS::Model::Scenario* scenario, BPMNOS::number currentTime = 0);

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
   * @brief Container holding a state machine for each running instance.
   */
  StateMachines instances; 

  /**
   * @brief Container holding all messages sent but not yet delivered.
   */
  Messages messages;

  auto_list<Token> tokensAwaitingReadyEvent; ///< Container holding all tokens awaiting a ready event

  auto_list<Token> tokensAwaitingRegularEntryEvent; ///< Container holding all tokens at regular activities awaiting an entry event

  //TODO: make sure that elements are deleted when no longer required
  std::unordered_map< Token*, auto_list<Token> > tokensAwaitingJobEntryEvent; ///< Map holding a container of all tokens awaiting entry at jobs for each token at an active resource

  auto_list<Token> tokensAtIdleResources; ///< Container holding indices of resources not executing a job and awaiting a job entry
//  std::vector< Token* > tokensAtActiveResources; ///< Container holding indices of tokens at busy resources

  auto_schedule<Token> tokensAwaitingTaskCompletionEvent; ///< Sorted container holding all tokens awaiting a task completion event

  auto_list<Token> tokensAwaitingChoiceEvent; ///< Container holding all tokens awaiting a choice event

  auto_list<Token> tokensAwaitingResourceShutdownEvent; ///< Container holding all tokens awaiting a choice event

  auto_list<Token> tokensAwaitingExitEvent; ///< Container holding all tokens awaiting an exit event

  auto_schedule<Token> tokensAwaitingTimer; ///< Sorted container holding holding all tokens awaiting a timer event

  auto_list<Token> tokensAwaitingMessageDelivery; ///< Container holding all tokens awaiting a message delivery event

  auto_list<Token> tokensAwaitingEventBasedGateway; ///< Container holding all tokens awaiting activation event for an event-based gateway

  //TODO: make sure that elements are deleted when no longer required
  std::unordered_map< const StateMachine*, std::vector<Token*> > tokensAwaitingStateMachineCompletion; ///< Map holding all tokens awaiting the completion of a state machine

  //TODO: make sure that elements are deleted when no longer required
  std::unordered_map< std::pair< const StateMachine*, const BPMN::FlowNode*>, std::vector<Token*>, PairHash > tokensAwaitingGatewayActivation; ///< Map holding tokens awaiting activation of a converging gateway 

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
