# Subprocesses and ad-hoc subprocesses
@page token_flow_logic_subprocesses Subprocesses and ad-hoc subprocesses

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

- @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
- @subpage token_flow_logic_compensation_activities "Compensation activities"

# Subprocesses and ad-hoc subprocesses (excluding multi-instance and compensation activities)

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
    BUSY --> COMPLETED
    BUSY --> FAILING: failure
    COMPLETED --> EXITING: exit event
    EXITING --> feasibleExit
    feasibleExit --> departure: [feasible]
    feasibleExit --> FAILING: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
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

Upon entry, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the (ad-hoc) subprocess token.
Then, feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise,
a token is created at the unique @ref BPMN::SubProcess::startEvent "start event" of the subprocess or each @ref BPMN::Process::startNodes "start node" of the ad-hoc subprocess,
at the @ref BPMN::EventSubProcess::startEvent "start event" of each @ref BPMN::EventSubProcess "event subprocess" (excluding those having a @ref BPMN::CompensateStartEvent "compensate start event"), and
at each @ref BPMN::BoundaryEvent "boundary event" (excluding @ref BPMN::CompensateBoundaryEvent "compensate boundary events").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the (ad-hoc) subprocess  token.
Furthermore, 
the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".


@attention All operators provided for @ref BPMN::SubProcess "subprocesses" and @ref BPMNOS::Model::SequentialAdHocSubProcess "ad-hocsubprocesses" must be instantaneous, i.e., they must not change the timestamp.
@par 
@note @ref BPMN::SubProcess "Subprocesses" must have a unique @ref BPMN::UntypedStartEvent "blank start event".
@par 
@note @ref BPMNOS::Model::SequentialAdHocSubProcess "Ad-hocsubprocesses" must not contain any start event and any flow node without incoming sequence flows is considered to be a @par 
@note @ref BPMN::EventSubProcess "Event-subprocesses" must have a unique @ref BPMN::TypedStartEvent "typed start event".
@ref BPMN::Process::startNodes "start node".
@par 
@attention A failure occurring at this stage can not yet be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

### BUSY

A token remains in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state until the process 
- is terminated due to an uncaught failure,
- is interrupted by an interrupting boundary event, or 
- the last active token within the scope has completed without failure.

While the token is in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state, 
the @ref BPMNOS::Execution::Token::status "token status" may be updated due to an escalation raised downstream by an @ref BPMN::EscalationThrowEvent "escalation event".
If the token resides at a (ad-hoc) subprocess which is the  @ref BPMNOS::Model::SequentialAdHoCSubProcess::performer "performer" of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess", 
the @ref BPMNOS::Execution::Token::status "token status" is also updated whenever a token exits a child activity of the @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess".

When an uncaught failure is raised within the scope of the (ad-hoc) subprocess, the @ref BPMNOS::Execution::Token::status "status" of the (ad-hoc) subprocess token is set to the @ref BPMNOS::Execution::Token::status "status" of the token raising the failure.
All tokens within the scope of the process (excluding those created for compensation) are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note A failure occurring at this stage may be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

When all tokens within the scope of the (ad-hoc) subprocess have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and no event subprocess is running, the @ref BPMNOS::Execution::Token::status "status" of the (ad-hoc) subprocess token is updated with the merged status of all these tokens within the scope. All child tokens are disposed  (including those at start events of  @ref BPMN::EventSubProcess "event subprocesses") before the @ref BPMNOS::Execution::Token::state "state" of the (ad-hoc) subprocess token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

An (ad-hoc) subprocess also completes if
 an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "interrupting" completes successfully.
 In such a case, the @ref BPMNOS::Execution::Token::status "token status" of the (ad-hoc) subprocess is analogously updated with the merged status of all tokens in @ref BPMNOS::Execution::Token::State::DONE "DONE" state within the scope of the interrupting event subprocess. All of these tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the (ad-hoc) subprocess token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".


### COMPLETED

A token in  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state waits for an @ref BPMNOS::Execution::ExitEvent "exit event" indicating that a decision is made to leave the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::EXITING "EXITING".


### EXITING
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note The failure occurring at this stage can no longer be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

Otherwise, the (ad-hoc) subprocess has been executed successfully.
In the case, that the (ad-hoc) subprocess has an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::CompensateStartEvent "compensate atart event", a token is created at this start event.
In the case, that the (ad-hoc) subprocess has a @ref BPMN::CompensateBoundaryEvent "compensate boundary event", a token is created at this boundary event.
If the (ad-hoc) subprocess has an outgoing sequence flow, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".


### DEPARTED

When the (ad-hoc) subprocess token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

### DONE

When the (ad-hoc) subprocess token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

### FAILING

When the process token reaches @ref BPMNOS::Execution::Token::State::FAILING "FAILING" state, 
the @ref BPMNOS::Execution::Token::status "status" of the process token is updated using the values of the token causing the failure,
all tokens within the scope of the process are withdrawn, 
and the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED" after all relevant activities within the scope of the process have been compensated.

### FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


