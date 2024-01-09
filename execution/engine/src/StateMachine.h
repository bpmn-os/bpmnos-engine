#ifndef BPMNOS_Execution_StateMachine_H
#define BPMNOS_Execution_StateMachine_H

#include <bpmn++.h>
#include "Token.h"
#include "model/utility/src/Number.h"
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
 */
class StateMachine : public std::enable_shared_from_this<StateMachine> {
public:
  StateMachine(const SystemState* systemState, const BPMN::Process* process);
  StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken);
  StateMachine(const StateMachine* other);
  ~StateMachine();

  const SystemState* systemState;
  const BPMN::Process* process; ///< Pointer to the top-level process.
  const BPMN::Scope* scope; ///< Pointer to the current scope.
  const std::string instanceId;
  Token* parentToken;

  Tokens tokens; ///< Container with all tokens within the scope of the state machine.
  StateMachines subProcesses; ///< Container with state machines of all active (sub)processes.
  std::shared_ptr<StateMachine> interruptingEventSubProcess; ///< State machines representing an active event subprocess that is interrupting.
  StateMachines nonInterruptingEventSubProcesses; ///< Container with state machines of all active event subprocesses that are not interrupting.
  StateMachines pendingEventSubProcesses; ///< Container with state machines of all inactive event subprocesses that may be triggered.

  Tokens compensationTokens; ///< Container with all tokens created for a compensation activity.
  StateMachines compensations; ///< Container with state machines for all compensations within scope.

  Token* findCompensationToken(BPMN::Node* compensationNode) const; ///< Returns the token for a compensation activity or compenmsation event subprocess, if no such token exists a `nullptr` is returned.
  void run(const Values& status); ///< Create initial token and advance it.

private:
  friend class Engine;
  friend class SystemState;
  friend class Token;

  static constexpr char delimiter = '^'; ///< Delimiter used for disambiguation of identifiers of non-interrupting event subprocesses
  std::map< const BPMN::FlowNode*, unsigned int > instantiations; ///< Instantiation counter for start events of non-interrupting event subprocesses

  void registerRecipient(); ///< Register new state machine to allow directed message delivery
  void unregisterRecipient(); ///< Register new state machine id to withdraw directed messages

  void createChild(Token* parent, const BPMN::Scope* scope); ///< Method creating the state machine for a (sub)process

  void createCompensationActivity(const BPMN::Activity* compensationActivity, const BPMNOS::Values& status); ///< Method creating the compensation activity of an activity
  void createCompensationEventSubProcess(const BPMN::EventSubProcess* compensationEventSubProcess, const BPMNOS::Values& status); ///< Method creating the compensation event subproces of an activity

  void createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status); ///< Method creating the state machine for an interrupting event subprocess

  void createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status); ///< Method creating the state machine for an non-interrupting event subprocess

  void initiateBoundaryEvents(Token* token); ///< Method placing tokens on all boundary events
  void initiateBoundaryEvent(Token* token, const BPMN::FlowNode*); ///< Method placing tokens on a boundary event
  void initiateEventSubprocesses(Token* token); ///< Method initiating pending event subprocesses

  void createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows);
  void createMergedToken(const BPMN::FlowNode* gateway);

  void shutdown();
  void interruptActivity(Token* token);

  void copyToken(Token* token);
  void handleEscalation(Token* token);
  void handleFailure(Token* token);
  void attemptGatewayActivation(const BPMN::FlowNode* node);
  void attemptShutdown();
  void deleteChild(StateMachine* child); ///< Method removing completed state machine from parent
  void deleteNonInterruptingEventSubProcess(StateMachine* eventSubProcess); ///< Method removing completed event subprocess from context
  void deleteTokensAwaitingBoundaryEvent(Token* token); ///< Method removing all waiting tokens attached to activity of token
  void advanceTokenWaitingForCompensation(const BPMN::Node* compensationNode); ///< Method advancing a token that was waiting for a compensation activity or compensation event subprocess
  void completeCompensationActivity(Token* token); ///< Method handling the completion of a compensation activity
};


} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StateMachine_H
