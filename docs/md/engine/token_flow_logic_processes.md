# Processes
@page token_flow_logic_processes Processes

A process instance is created when all data required for the @ref BPMNOS::Model::ExtensionElements::attributes "attributes" defined for the @ref BPMN::Process "process" element, in particular, the instance identifier and the timestamp, become known and the timestamp is reached.
Upon instantiation, a @ref BPMNOS::Execution::Token "token" is created which resides at the process element.

## States
A token at a process element changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    [*] --> ENTERED
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED
    BUSY --> FAILING: failure
    COMPLETED --> feasibleExit
    feasibleExit --> DONE: [feasible]
    feasibleExit --> FAILING: [infeasible]
    DONE --> [*]
    FAILING --> FAILED
    FAILED --> [*]
</pre>


### Token creation

After initialization, the @ref BPMNOS::Execution::Token::state "state" of the process token is immediately set to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED". 

### ENTERED
Upon entry, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the process token.
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, 
a token is created at the @ref BPMN::Process::startNodes "start node" of the process
and at the @ref BPMN::EventSubProcess::startEvent "start event" of each @ref BPMN::EventSubProcess "event subprocess" (excluding those having a @ref BPMN::CompensateStartEvent "compensate start event").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the process token.
Furthermore, 
the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY" and 
@attention All operators must be instantaneous, i.e., they must not change the timestamp.
@par 
@attention It is assumed that each process has a unique @ref BPMN::UntypedStartEvent "blank start event".
@par 
@note @ref BPMN::EventSubProcess "Event-subprocesses" must have a unique @ref BPMN::TypedStartEvent "typed start event".
@par 
@note A failure occurring at this stage can not yet be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

### BUSY
A process token remains in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state until the process 
- is terminated due to an uncaught failure, or 
- the last active token within the scope has completed without failure.

While the process token is in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state, 
the @ref BPMNOS::Execution::Token::status "token status" may be updated due to an escalation raised downstream by an @ref BPMN::EscalationThrowEvent "escalation event".
If the token resides at a process which is the  @ref BPMNOS::Model::SequentialAdHoCSubProcess::performer "performer" of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess", 
the @ref BPMNOS::Execution::Token::status "token status" is also updated whenever a token exits a child activity of the @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess".

When an uncaught failure is raised within the scope of the process, the @ref BPMNOS::Execution::Token::status "status" of the process token is set to the @ref BPMNOS::Execution::Token::status "status" of the token raising the failure.
All tokens within the scope of the process (excluding those created for compensation) are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note A failure occurring at this stage may be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

When all tokens within the scope of the process have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and no event subprocess is running, the @ref BPMNOS::Execution::Token::status "status" of the process token is updated with the merged status of all these tokens within the scope. All child tokens are disposed  (including those at start events of  @ref BPMN::EventSubProcess "event subprocesses") before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

A process also completes if
 an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "interrupting" completes successfully.
 In such a case, the @ref BPMNOS::Execution::Token::status "token status" of the process is analogously updated with the merged status of all tokens in @ref BPMNOS::Execution::Token::State::DONE "DONE" state within the scope of the interrupting event subprocess. All of these tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".



### COMPLETED
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If none of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note The failure occurring at this stage can no longer be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

### DONE

When the process token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, it is disposed and the process instance has successfully completed.

### FAILING

When the process token reaches @ref BPMNOS::Execution::Token::State::FAILING "FAILING" state, 
the @ref BPMNOS::Execution::Token::status "status" of the process token is updated using the values of the token causing the failure,
all tokens within the scope of the process are withdrawn, 
and the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED" after all relevant activities within the scope of the process have been compensated.

### FAILED

Upon failure, the process token is disposed.

