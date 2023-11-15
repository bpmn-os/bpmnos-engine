#ifndef BPMNOS_Execution_StateMachine_H
#define BPMNOS_Execution_StateMachine_H

#include <bpmn++.h>
#include "Token.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {


class StateMachine;
typedef std::vector< std::unique_ptr<StateMachine> > StateMachines;

class SystemState;

/**
 * @brief Represents a state machine for BPMN execution of a scope in the model.
 *
 * This class manages all tokens for BPMN execution of a given scope. It also holds
 * a state machine for each child scope instantiated.
 */
class StateMachine {
public:
  StateMachine(const SystemState* systemState, const BPMN::Process* process);
  StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken);
  const SystemState* systemState;
  const BPMN::Process* process; ///< Pointer to the top-level process.
  const BPMN::Scope* scope; ///< Pointer to the current scope.
  Token* parentToken;

  Tokens tokens; ///< Container with all tokens within the scope of the state machine.
  StateMachines subProcesses; ///< Container with state machines of all active (sub)processes.
  std::unique_ptr<StateMachine> interruptingEventSubProcess; ///< State machines representing an active event subprocess that is interrupting.
  StateMachines nonInterruptingEventSubProcesses; ///< Container with state machines of all active event subprocesses that are not interrupting.
  StateMachines pendingEventSubProcesses; ///< Container with state machines of all inactive event subprocesses that may be triggered.


  void run(const Values& status); ///< Create initial token and advance it.
  bool isCompleted() const;

private:
  friend class Engine;
  friend class SystemState;
  friend class Token;
  /**
   * @brief Set token state and advance token as much as possible.
   */
  void advanceToken(Token* token, Token::State state);

  void createChild(Token* parentToken, const BPMN::Scope* scope); ///< Method creating the state machine for a (sub)process

  void createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows);
  void createMergedToken(std::unordered_map< std::pair<const StateMachine*, const BPMN::FlowNode*>, std::vector<Token*> >::iterator gatewayIt);
  void shutdown(std::unordered_map< const StateMachine*, std::vector<Token*> >::iterator it);
};


} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StateMachine_H
