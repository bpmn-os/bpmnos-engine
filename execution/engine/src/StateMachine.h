#ifndef BPMNOS_Execution_StateMachine_H
#define BPMNOS_Execution_StateMachine_H

#include <bpmn++.h>
#include "Token.h"
#include "model/utility/src/Value.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

class StateMachine;
typedef std::vector< std::shared_ptr<StateMachine> > StateMachines;

class SystemState;

/**
 * @brief Represents a state machine for BPMN execution of a scope in the model.
 *
 * This class manages all tokens for BPMN execution of a given scope.
 *
 * @par Ownership Hierarchy
 * The global state machine of a system state has no scope and no @ref parentToken. Its tokens are the
 * process-level tokens, one for each process instance, whose node is the process. Each of them owns the
 * state machine of its process instance, which holds the data of the instance and the flow tokens, and
 * which is the @ref root of every state machine within the instance. Subprocesses follow the same pattern:
 * a token at the subprocess node owns a child state machine with the subprocess's flow tokens.
 * @note Inclusive gateways are not yet supported.
 *
 * @attention Event subprocesses within event subprocesses are not yet tested (and may not be supported).
 */
class StateMachine : public std::enable_shared_from_this<StateMachine> {
public:
  static constexpr char delimiters[] = {'^','#'}; ///< Delimiters used for disambiguation of identifiers of non-interrupting event subprocesses, multi-instance activities and instances created by a trigger
  /**
   * @brief Constructs the global state machine of a system state, which holds a token for each process
   * instance and has neither a scope nor a parent token. Its data are the global attributes, which every
   * state machine inherits as the first part of its data.
   *
   * @param systemState The system state this state machine belongs to
   * @param globals The values of the global attributes
   */
  StateMachine(const SystemState* systemState, Data globals);

  /**
   * @brief Constructs a child StateMachine for a scope within a process.
   *
   * Creates a state machine for a subprocess, event subprocess, or other nested scope.
   * The state machine inherits data from its parent token's owner.
   *
   * @param systemState The system state this state machine belongs to
   * @param scope The BPMN scope (subprocess, event subprocess, etc.)
   * @param parentToken The token that owns this child state machine
   * @param dataAttributes Data attribute values owned by this scope
   * @param instance Optional instance identifier (defaults to parent's instance)
   */
  StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken, Data dataAttributes, std::optional<BPMNOS::number> instance = std::nullopt);

  /**
   * @brief Respawn constructor for non-interrupting event subprocesses.
   *
   * Creates a copy of a pending event subprocess within the same execution context.
   * Used when a non-interrupting event subprocess is triggered: the triggered instance
   * moves to nonInterruptingEventSubProcesses, and this copy replaces it in
   * pendingEventSubProcesses so the event can be triggered again.
   *
   * @param other The pending event subprocess to respawn
   */
  StateMachine(const StateMachine* other);

  /**
   * @brief Deep copy constructor for cloning to a different SystemState.
   *
   * Creates a deep copy of the state machine that belongs to a new SystemState.
   * All simple values and owned data are copied. Tokens and child state machines
   * are NOT copied by this constructor (handled in subsequent copy phases).
   *
   * @param systemState The new system state this copy belongs to
   * @param parentToken The new parent token (nullptr for the global state machine)
   * @param other The source StateMachine to copy from
   * @param context The copied context of an event subprocess, which copies its event subprocesses before the
   *        parent token owns it (nullptr for any other state machine)
   */
  StateMachine(const SystemState* systemState, Token* parentToken, const StateMachine* other, const StateMachine* context = nullptr);

  ~StateMachine();

  Data getData(const BPMN::Scope* scope);

  const SystemState* systemState; ///< Pointer to the system state this state machine belongs to.
  const BPMN::Scope* scope; ///< Pointer to the current scope (nullptr for the global state machine).
  const StateMachine* root; ///< Pointer to the state machine of the process instance, whose scope is the process (nullptr for the global state machine)
  std::optional<BPMNOS::number> instance; ///< Numeric representation of instance id (TODO: can we const this?)

  Token* parentToken; ///< Token that owns this state machine (nullptr for the global state machine).
  Data ownedData; ///< Container holding data attributes owned by the state machine.
  SharedData data; ///< Container holding references to all data attributes.

  Tokens tokens; ///< Container with all tokens within the scope of the state machine.
  std::shared_ptr<StateMachine> interruptingEventSubProcess; ///< State machines representing an active event subprocess that is interrupting.
  StateMachines nonInterruptingEventSubProcesses; ///< Container with state machines of all active event subprocesses that are not interrupting.
  StateMachines pendingEventSubProcesses; ///< Container with state machines of all inactive event subprocesses that may be triggered.

  Tokens compensationTokens; ///< Container with all tokens created for a compensation activity.
  StateMachines compensationEventSubProcesses; ///< Container with state machines created for a compensation event subprocesses of a child subprocess
  Tokens compensableSubProcesses; ///< Container holding tokens owning completed subprocesses with a compensation event subprocess

  Tokens getCompensationTokens(const BPMN::Activity* activity = nullptr) const; ///< Returns the compensation tokens for a given activity or for all activities
  void run(Status status); ///< Create the initial tokens of the scope of a child state machine and advance them.

private:
  friend class Engine;
  friend class SystemState;
  friend class Token;

  std::map< const BPMN::FlowNode*, unsigned int > instantiations; ///< Instantiation counter for start events of non-interrupting event subprocesses

  void registerRecipient(); ///< Register new state machine to allow directed message delivery
  void unregisterRecipient(); ///< Withdraw the directed messages of a non-interrupting event subprocess
  void withdrawMessages(); ///< Withdraw the messages directed to the state machine

  /// @brief Method creating a process-level token in the global state machine, together with the state
  /// machine of the process instance it owns, which holds the data of the instance.
  Token* createInstance(const BPMN::Process* process, Data data, Status status);

  void updateObjective(); ///< Updates the objective by the values the data attributes of the scope are created with, these never passing through @ref BPMNOS::Model::AttributeRegistry::setValue
  void createChild(Token* parent, const BPMN::Scope* scope, Data data, std::optional<BPMNOS::number> instance = std::nullopt); ///< Method creating the state machine for a (sub)process
  Data undefinedData(const BPMN::Node* node) const; ///< Returns undefined values for the data attributes the node owns, and its default objects
  static void takeTriggeringStatus(Token* eventToken, const Status& status); ///< Hands the status of the token triggering an event subprocess to its start token, leaving the attributes of the event subprocess undefined

  void createCompensationTokenForBoundaryEvent(const BPMN::BoundaryEvent* compensateBoundaryEvent, BPMNOS::Status status); ///< Method creating a compensation token at a compensate boundary event of an activity
//  void createCompensationTokenForEventSubProcess(const BPMN::EventSubProcess* compensationEventSubProcess, Token* token); ///< Method creating a compensation token at a compensation event subproces of an activity

  void createCompensationEventSubProcess(const BPMN::EventSubProcess* eventSubProcess, BPMNOS::Status status); ///< Method creating the compensation event subproces of an activity

//  void createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Status& status); ///< Method creating the state machine for an interrupting event subprocess

//  void createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Status& status); ///< Method creating the state machine for an non-interrupting event subprocess

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
