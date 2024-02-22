# Activities
@page token_flow_logic_activities Activities

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

- @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
- @subpage token_flow_logic_compensation_activities "Compensation activities"

# Activities (excluding multi-instance and compensation activities)

## States
A token at an activity (excluding multi-instance and compensation activities) changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    state departure <<choice>>
    state failure <<choice>>
    [*] --> ARRIVED
    ARRIVED --> READY: ready event
    READY --> ENTERED: entry event
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED
    BUSY --> failure: failure
    COMPLETED --> EXITING: exit event
    EXITING --> feasibleExit
    feasibleExit --> departure: [feasible]
    feasibleExit --> failure: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    failure --> FAILING: [(ad-hoc) subprocess]
    failure --> FAILED: [task]
    FAILING --> FAILED
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

#### Subprocesses and ad-hoc subprocesses

Upon entry, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the subprocess token.
Furthermore, a token is created at the unique @ref BPMN::SubProcess::startEvent "start event" of the subprocess or each each @ref BPMN::Process::startNodes "start node" of the ad-hoc subprocess. 
This token inherits the @ref BPMNOS::Execution::Token::status "status attributes" of the subprocess token.
Then, the @ref BPMNOS::Execution::Token::state "state" of the subprocess token is set to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED". 
@attention All operators provided for @ref BPMN::SubProcess "subprocesses" or @ref BPMNOS::Model::SequentialAdHocSubProcess "ad-hocsubprocesses" must be instantaneous, i.e., they must not change the timestamp.
@par 
@note @ref BPMN::SubProcess "Subprocesses" must have a unique @ref BPMN::UntypedStartEvent "blank start event" and @ref BPMNOS::Model::SequentialAdHocSubProcess "ad-hocsubprocesses" must not contain any start event and any flow node without incoming sequence flows is considered to be a start node.


Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If none of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

@note The failure occurring at this stage can not yet be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

#### Tasks

### BUSY
A new token is created at the unique start node of the process.
This token inherits the @ref BPMNOS::Execution::Token::status "status attributes" of the original token.
The process token remains in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state as long as the process is neither terminated due to an uncaught failure nor successfully completed.

In the case that

- all tokens within the scope of the process have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and no event subprocess is running, or
- the last @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "not interrupting" completes successfully after all tokens within the scope of the process have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state,

the @ref BPMNOS::Execution::Token::status "status" of the process token is updated with the merged status of all tokens within its scope. All child tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

In the case that an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "interrupting" completes successfully, the @ref BPMNOS::Execution::Token::status "token status" of the process is analogously updated with the merged status of all tokens in @ref BPMNOS::Execution::Token::State::DONE "DONE" state within the scope of the interupting event subprocess. All of these tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

In case of an uncaught failure, the @ref BPMNOS::Execution::Token::state "state" of the original token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

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


