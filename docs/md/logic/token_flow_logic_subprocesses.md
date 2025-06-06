# Subprocesses and ad-hoc subprocesses
@page token_flow_logic_subprocesses Subprocesses and ad-hoc subprocesses

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not and whether the activity is for compensation or not.

- @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
- @subpage token_flow_logic_compensation_activities "Compensation activities"

# Subprocesses and ad-hoc subprocesses (excluding multi-instance and compensation activities)

A token at a subprocess or ad-hoc subprocess changes its state as follows:

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
    BUSY --> COMPLETED
    BUSY --> FAILING: failure
    COMPLETED --> EXITING: exit event
    EXITING --> feasibleExit
    feasibleExit --> departure: [feasible]
    note left of departure
      In case of @ref XML::bpmn::tStandardLoopCharacteristics  "loop activities", the token may return to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state
      after receipt of the respective @ref BPMNOS::Execution::EntryEvent "entry event".
    end note
    feasibleExit --> FAILING: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILING --> FAILED
    FAILED --> [*]
</pre>


## ARRIVED

A token in  @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::READY "READY".


## READY

A token in  @ref BPMNOS::Execution::Token::State::READY "READY" state waits for an @ref BPMNOS::Execution::EntryEvent "entry event" indicating that a decision is made to start with the activity.
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

## ENTERED
Upon entry, the initial assignment of @ref BPMNOS::Model::Attribute "attributes" are conducted in the order of attribute definitions.
Then, feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise,
a token is created at the unique @ref BPMN::SubProcess::startEvent "start event" of the subprocess or each @ref BPMN::Process::startNodes "start node" of the ad-hoc subprocess,
at the @ref BPMN::EventSubProcess::startEvent "start event" of each @ref BPMN::EventSubProcess "event subprocess" (excluding those having a @ref BPMN::CompensateStartEvent "compensate start event"), and
at each @ref BPMN::BoundaryEvent "boundary event" (excluding @ref BPMN::CompensateBoundaryEvent "compensate boundary events").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the (ad-hoc) subprocess  token.
Furthermore,
the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".

@note @ref BPMN::SubProcess "Subprocesses" must have a unique @ref BPMN::UntypedStartEvent "blank start event".
@par
@note @ref BPMNOS::Model::SequentialAdHocSubProcess "Ad-hoc subprocesses" must not contain any start event and any flow node without incoming sequence flows must be a an @ref BPMN::Activity "activity".
@par
@note @ref BPMN::EventSubProcess "Event-subprocesses" must have a unique @ref BPMN::TypedStartEvent "typed start event".
@par
@attention A failure occurring at this stage can not yet be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

## BUSY

A token remains in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state until the process
- is terminated due to an uncaught failure,
- is interrupted by an interrupting boundary event, or
- the last active token within the scope has completed without failure.

While the token is in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state,
the @ref BPMNOS::Execution::Token::status "token status" may be updated due to an escalation raised downstream by an @ref BPMN::EscalationThrowEvent "escalation event".
If the token resides at a (ad-hoc) subprocess which is the  @ref BPMNOS::Model::SequentialAdHocSubProcess::performer "performer" of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess",
the @ref BPMNOS::Execution::Token::status "token status" is also updated whenever a token exits a child activity of the @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess".

When an uncaught failure is raised within the scope of the (ad-hoc) subprocess, the @ref BPMNOS::Execution::Token::status "status" of the (ad-hoc) subprocess token is set to the @ref BPMNOS::Execution::Token::status "status" of the token raising the failure.
All tokens within the scope of the process (excluding those created for compensation) are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note A failure occurring at this stage may be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

When all tokens within the scope of the (ad-hoc) subprocess have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and no event subprocess is running, the @ref BPMNOS::Execution::Token::status "status" of the (ad-hoc) subprocess token is updated with the merged status of all these tokens within the scope. All child tokens are disposed  (including those at start events of  @ref BPMN::EventSubProcess "event subprocesses") before the @ref BPMNOS::Execution::Token::state "state" of the (ad-hoc) subprocess token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

An (ad-hoc) subprocess also completes if
 an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "interrupting" completes successfully.
 In such a case, the @ref BPMNOS::Execution::Token::status "token status" of the (ad-hoc) subprocess is analogously updated with the merged status of all tokens in @ref BPMNOS::Execution::Token::State::DONE "DONE" state within the scope of the interrupting event subprocess. All of these tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the (ad-hoc) subprocess token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".


## COMPLETED

When a token at the (ad-hoc) subprocess with an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::CompensateStartEvent "compensate start event" reaches  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state, a token is created at this start event.
If the (ad-hoc) subprocess has a @ref BPMN::CompensateBoundaryEvent "compensate boundary event", a token is created at this boundary event.

A token in  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state waits for an @ref BPMNOS::Execution::ExitEvent "exit event" indicating that a decision is made to leave the activity.
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::EXITING "EXITING".


## EXITING
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

@note The failure occurring at this stage can no longer be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event", but an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::CompensateStartEvent "compensate start event" can be triggered. 

Otherwise, the (ad-hoc) subprocess has been executed successfully and all tokens at boundary events (except @ref BPMN::CompensateBoundaryEvent "compensate boundary events") of the (ad-hoc) subprocess are withdrawn.

If the (ad-hoc) subprocess has an outgoing sequence flow, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".
In case of @ref XML::bpmn::tStandardLoopCharacteristics  "loop activities", the token may advance to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state after the respective entry decision has been made for the next loop of the activity.

## DEPARTED

When the (ad-hoc) subprocess token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

## DONE

When the (ad-hoc) subprocess token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

## FAILING

When the process token reaches @ref BPMNOS::Execution::Token::State::FAILING "FAILING" state,
the @ref BPMNOS::Execution::Token::status "status" of the process token is updated using the values of the token causing the failure,
all tokens within the scope of the process are withdrawn,
and the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED" after all relevant activities within the scope of the process have been compensated.

## FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.
