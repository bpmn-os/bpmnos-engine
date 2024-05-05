#ifndef BPMNOS_Execution_StateMachine_H
#define BPMNOS_Execution_StateMachine_H

#include <bpmn++.h>
#include "Token.h"
#include "model/utility/src/Number.h"
#include "model/data/src/Scenario.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

class StateMachine;
typedef std::vector< std::shared_ptr<StateMachine> > StateMachines;

class SystemState;

/**
 * @brief Represents a state machine for BPMN execution of a scope in the model.
 *
 * This class manages all tokens for BPMN execution of a given scope. It also holds
 * a state machine for each child scope instantiated.
 *
 * @note A state machine without @ref parentToken represents a @ref BPMN::Process.
 * @par
 * @note Inclusive gateways are not yet supported.
 * 
 * @attention Event subprocesses within event subprocesses are not yet tested (and may not be supported).
 */
class StateMachine : public std::enable_shared_from_this<StateMachine> {
public:
  StateMachine(const SystemState* systemState, const BPMN::Process* process, Values dataAttributes);
  StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken, Values dataAttributes, std::optional<BPMNOS::number> instance = std::nullopt);
  StateMachine(const StateMachine* other);
  ~StateMachine();

  Values getData(const BPMN::Scope* scope);

  const SystemState* systemState;
  const BPMN::Process* process; ///< Pointer to the top-level process.
  const BPMN::Scope* scope; ///< Pointer to the current scope.
  const StateMachine* root; ///< Pointer to the root state machine
  std::optional<BPMNOS::number> instance; ///< Numeric representation of instance id (TODO: can we const this?)

  Token* parentToken;
  Values ownedData; ///< Container holding data attributes owned by the state machine.
  SharedValues data; ///< Container holding references to all data attributes.

  Tokens tokens; ///< Container with all tokens within the scope of the state machine.
  std::shared_ptr<StateMachine> interruptingEventSubProcess; ///< State machines representing an active event subprocess that is interrupting.
  StateMachines nonInterruptingEventSubProcesses; ///< Container with state machines of all active event subprocesses that are not interrupting.
  StateMachines pendingEventSubProcesses; ///< Container with state machines of all inactive event subprocesses that may be triggered.

  Tokens compensationTokens; ///< Container with all tokens created for a compensation activity.
  StateMachines compensationEventSubProcesses; ///< Container with state machines created for a compensation event subprocesses of a child subprocess
  StateMachines compensableSubProcesses; ///< Container holding state machines for completed subprocesses with a compensation event subprocess and compensation tokens

  Tokens getCompensationTokens(const BPMN::Activity* activity = nullptr) const; ///< Returns the compensation tokens for a given activity or for all activities
  void run(Values status); ///< Create initial token and advance it.

private:
  friend class Engine;
  friend class SystemState;
  friend class Token;

  std::map< const BPMN::FlowNode*, unsigned int > instantiations; ///< Instantiation counter for start events of non-interrupting event subprocesses

  void registerRecipient(); ///< Register new state machine to allow directed message delivery
  void unregisterRecipient(); ///< Register new state machine id to withdraw directed messages

  void createChild(Token* parent, const BPMN::Scope* scope, Values data, std::optional<BPMNOS::number> instance = std::nullopt); ///< Method creating the state machine for a (sub)process

  void createCompensationTokenForBoundaryEvent(const BPMN::BoundaryEvent* compensateBoundaryEvent, BPMNOS::Values status); ///< Method creating a compensation token at a compensate boundary event of an activity
//  void createCompensationTokenForEventSubProcess(const BPMN::EventSubProcess* compensationEventSubProcess, Token* token); ///< Method creating a compensation token at a compensation event subproces of an activity

  void createCompensationEventSubProcess(const BPMN::EventSubProcess* eventSubProcess, BPMNOS::Values status); ///< Method creating the compensation event subproces of an activity

//  void createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status); ///< Method creating the state machine for an interrupting event subprocess

//  void createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status); ///< Method creating the state machine for an non-interrupting event subprocess

  void initiateBoundaryEvents(Token* token); ///< Method placing tokens on all boundary events
  void initiateBoundaryEvent(Token* token, const BPMN::FlowNode*); ///< Method placing tokens on a boundary event
  void initiateEventSubprocesses(Token* token); ///< Method initiating pending event subprocesses
  void createMultiInstanceActivityTokens(Token* token); ///< Method creating tokens for multi-instance activities
  void deleteMultiInstanceActivityToken(Token* token); ///< Method creating tokens for multi-instance activities
  void deleteAdHocSubProcessToken(Token* token);
  void compensateActivity(Token* token); ///< Method creating the compensation activity of an activity

  std::vector<Token*> createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows);
  void createMergedToken(const BPMN::FlowNode* gateway);

  void shutdown(); ///< Shutdown state machine after successful execution
  void interruptActivity(Token* token);
  void clearObsoleteTokens();

  void handleDivergingGateway(Token* token);
  void handleEventBasedGatewayActivation(Token* token);
  void handleEscalation(Token* token);
  void handleFailure(Token* token);
  void attemptGatewayActivation(const BPMN::FlowNode* node);
  void attemptShutdown();
//  void deleteChild(StateMachine* child); ///< Method removing completed state machine from parent
  void deleteNonInterruptingEventSubProcess(StateMachine* eventSubProcess); ///< Method removing completed event subprocess from context
  void deleteCompensationEventSubProcess(StateMachine* eventSubProcess); ///< Method removing completed compensation event subprocess from context
  void deleteTokensAwaitingBoundaryEvent(Token* token); ///< Method removing all waiting tokens attached to activity of token
  void completeCompensationActivity(Token* token); ///< Method handling the completion of a compensation activity
  void completeCompensationEventSubProcess(); ///< Method handling the completion of a compensation event subprocess
  void advanceTokenWaitingForCompensation(Token* waitingToken); ///< Method advancing a token that was waiting for a compensation to be completed
  void compensate(Tokens compensations, Token* waitingToken); ///< Method compensating all activities in reverse order before the waiting token may advance

  Token* findTokenAwaitingErrorBoundaryEvent(Token* activityToken); ///< Method finding the token at a boundary event catching an error thrown in activity 
};


} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StateMachine_H
