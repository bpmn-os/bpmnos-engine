# Tasks
@page token_flow_logic_tasks Tasks

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 


# Tasks (excluding multi-instance and compensation activities)

## States
A token at an activity (excluding multi-instance and compensation activities) changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    state departure <<choice>>
    [*] --> ARRIVED
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

@note The @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state is not relevant for activities within @ref BPMNOS::Model::SequentialAdHocSubProcess "ad-hoc subprocesses" which do not have any incoming sequence flows. 
For such nodes, a token is created when the parent is entered. When a @ref BPMNOS::Execution::ReadyEvent "ready event" is received the state of this token is updated to @ref BPMNOS::Execution::Token::State::READY "READY" state.

### ARRIVED

A token in  @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::READY "READY".


### READY

A token in  @ref BPMNOS::Execution::Token::State::READY "READY" state waits for an @ref BPMNOS::Execution::EntryEvent "entry event" indicating that a decision is made to start with the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

### ENTERED

Upon entry, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to determine an expected status update and the @ref BPMNOS::Execution::Token::state "state" of the token is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".
@note The actual @ref BPMNOS::Execution::Token::status "status" of the token is not updated before completion of the task.


### BUSY
A token in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state waits for a @ref BPMNOS::Execution::CompletionEvent "completion event" before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

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


