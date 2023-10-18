#ifndef BPMNOS_Execution_StateMachine_H
#define BPMNOS_Execution_StateMachine_H

#include <bpmn++.h>
#include "Token.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {


class StateMachine;
typedef std::vector< StateMachine > StateMachines;

/**
 * @brief Represents a state machine for BPMN execution of a scope in the model.
 *
 * This class manages all tokens for BPMN execution of a given scope. It also holds
 * a state machine for each child scope instantiated.
 */
class StateMachine {
public:
  StateMachine(const BPMN::Scope* scope, const Values& status, Token* parentToken = nullptr);
  const BPMN::Scope* scope;
  Token* parentToken;

  Tokens tokens; ///< Container with all tokens within the scope of the state machine.
  StateMachines childInstances; ///< Container with state machines of all child scopes.
  StateMachines eventSubprocessInstances; ///< Container with state machines representing instances of event subprocess.

  bool isCompleted() const;

private:
  friend class Engine;
  /**
   * @brief Advance all tokens as much as possible.
   */
  void advance();
  /**
   * @brief Advance token as much as possible.
   */
  void advance(Token& token);

  /// TODO: still needed?
//  bool run(const Event* event); ///< Process event and advance tokens as much as possible. Returns false if state machine represents a completed instance.

  /// TODO: still needed?
//  bool run(Token* token); ///< Advance tokens as much as possible. Returns false if state machine represents a completed instance.


  /// TODO: still needed?
//  void continueWithParentToken();
};


} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_StateMachine_H
