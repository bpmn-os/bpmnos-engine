#ifndef BPMNOS_Execution_SystemState_H
#define BPMNOS_Execution_SystemState_H

#include "StateMachine.h"
#include "execution/engine/src/Message.h"
#include "model/data/src/Scenario.h"

namespace BPMNOS::Execution {

/**
 * @brief A class representing the state that the execution or simulation of a given scenario is in.
 */
class SystemState {
public:
  /**
   * @brief Timestamp holding the point in time that the engine is in (this is usually representing now).
   */
  BPMNOS::number currentTime;

  /**
   * @brief Timestamp holding the point in time that the simulation is in (this could be a future point in time).
   */
  std::optional<BPMNOS::number> assumedTime;

  /**
   * @brief Pointer to the corresponding scenario. 
   */
  const BPMNOS::Model::Scenario* scenario;

  /**
   * @brief Function returning the assumed time time if available or the current time otherwise.
   */
  BPMNOS::number getTime() const;

  /**
   * @brief Function returning true if the system is still alive and false if everything is completed.
   */
  bool isAlive() const { return false; }; // TODO

  /**
   * @brief Method returning a vector of all instantiations at the given time.
   */
  std::vector< std::pair<const BPMN::Process*, BPMNOS::Values> > getInstantiations() const;

  /**
   * @brief Container holding a state machine for each instance.
   */
  StateMachines instances; 

  /**
   * @brief Container holding all messages sent but not yet delivered.
   */
  Messages messages; 

  std::vector<Token*> awaitingReady; ///< Container holding all tokens awaiting a ready event
  std::vector<Token*> awaitingRegularEntry; ///< Container holding all tokens at regular activities awaiting an entry event
  std::vector<Token*> awaitingJobEntry; ///< Container holding all tokens at jobs awaiting an entry event
  std::vector<Token*> awaitingChoice; ///< Container holding all tokens awaiting a choice event
  std::vector<Token*> awaitingTimer; ///< Container holding all tokens at a catching timer event awaiting a trigger event
  std::vector<Token*> awaitingMessageDelivery; ///< Container holding all tokens awaiting a message delivery event
  std::vector<Token*> awaitingCompletion; ///< Container holding all tokens awaiting a completion event
  std::vector<Token*> awaitingExit; ///< Container holding all tokens awaiting an exit event

private:
  friend class Engine;
  void incrementTimeBy(BPMNOS::number duration);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SystemState_H
