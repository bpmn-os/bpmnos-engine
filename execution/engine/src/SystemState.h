#ifndef BPMNOS_SystemState_H
#define BPMNOS_SystemState_H

#include "StateMachine.h"
#include "Message.h"
#include "events/InstantiationEvent.h"
#include "model/data/src/Scenario.h"

namespace BPMNOS::Execution {

struct SystemState {
  BPMNOS::number currentTime; ///< Timestamp holding the point in time that the engine is in (this is usually representing now)
  std::optional<BPMNOS::number> assumedTime; ///< Timestamp holding the point in time that the simulation is in (this could be a future point in time)
  const BPMNOS::Model::Scenario* scenario;
  BPMNOS::number getTime() const; ///< Function returning the assumed time time if available or the current time otherwise

  StateMachines instances; ///< Container holding a state machine for each instance
  Messages messages; ///< Container holding all messages sent but not yet delivered

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
  StateMachine* addStateMachine(const InstantiationEvent* event);
  void incrementTimeBy(BPMNOS::number duration);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_SystemState_H
