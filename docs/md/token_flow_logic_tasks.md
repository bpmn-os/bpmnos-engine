# Tasks
@page token_flow_logic_tasks Tasks

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

- @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
- @subpage token_flow_logic_compensation_activities "Compensation activities"

# Tasks (excluding multi-instance and compensation activities)

## States
A token at an activity (excluding multi-instance and compensation activities) changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    state departure <<choice>>
    [*] --> ARRIVED
    note left of ARRIVED
      If an activity does not have any incoming sequence flows,
      the @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state is skipped 
    end note
    ARRIVED --> READY: ready event
    READY --> ENTERED: entry event
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED: completion event
    BUSY --> FAILED: failure
    COMPLETED --> EXITING: exit event
    EXITING --> feasibleExit
    feasibleExit --> departure: [feasible]
    feasibleExit --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>


### ARRIVED

A token in  @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::READY "READY".


### READY

A token in  @ref BPMNOS::Execution::Token::State::READY "READY" state waits for an @ref BPMNOS::Execution::EntryEvent "entry event" indicating that a decision is made to start with the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

### ENTERED

Upon entry, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to determine an expected status update and the @ref BPMNOS::Execution::Token::state "state" of the token is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".
@note The actual @ref BPMNOS::Execution::Token::status "status" of the token is not updated before completion of the task.

A token is created at each @ref BPMN::BoundaryEvent "boundary event" (excluding @ref BPMN::CompensateBoundaryEvent "compensate boundary events").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the (ad-hoc) subprocess  token.
Furthermore, 
the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".


@todo @ref BPMNOS::Model::DecisionTask operators must be instantaneous and are applied after the choices have been mad
@par
@todo @ref BPMN::ReceiveTask  operators must be instantaneous and are applied after the message is received
@par
@todo @ref BPMN::SendTask operators must be instantaneous and are applied before the message is sent


### BUSY
A token in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state waits for a @ref BPMNOS::Execution::CompletionEvent "completion event" before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

### COMPLETED

A token in  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state waits for an @ref BPMNOS::Execution::ExitEvent "exit event" indicating that a decision is made to leave the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::EXITING "EXITING".


### EXITING
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

Otherwise, the task has been executed successfully and all tokens at boundary events of the task are withdrawn.
In the case, that the task has a @ref BPMN::CompensateBoundaryEvent "compensate boundary event", a token is created at this boundary event.
If the task has an outgoing sequence flow, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".

### DEPARTED

When the token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

### DONE

When the token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

### FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


