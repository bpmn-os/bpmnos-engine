# Tasks
@page token_flow_logic_tasks Tasks

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not and whether the activity is for compensation or not. 

- @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
- @subpage token_flow_logic_compensation_activities "Compensation activities"

# Tasks (excluding multi-instance and compensation activities)

A token at a task changes its state as follows:

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
    note left of departure
      In case of @ref XML::bpmn::tStandardLoopCharacteristics  "loop activities", the token may return to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state
      after receipt of the respective @ref BPMNOS::Execution::EntryEvent "entry event".
    end note
    feasibleExit --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>


## ARRIVED

A token in  @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::READY "READY".


## READY

A token in  @ref BPMNOS::Execution::Token::State::READY "READY" state waits for an @ref BPMNOS::Execution::EntryEvent "entry event" indicating that a decision is made to start with the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

## ENTERED

Upon entry at a task, a token is created at each @ref BPMN::BoundaryEvent "boundary event" (excluding @ref BPMN::CompensateBoundaryEvent "compensate boundary events").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the task token.
Then, feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".




## BUSY


For a task which is not a @ref BPMN::ReceiveTask  "receive task" or @ref BPMNOS::Model::DecisionTask "decision task",
 the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the token.
 
If the token resides at a @ref BPMN::SendTask "send task", a @ref BPMNOS::Execution::Message "message" is created after application of the operators and
the token waits for a @ref BPMNOS::Execution::MessageDeliveryEvent "message delivery event" at any of the recipients before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

A token at a @ref BPMN::ReceiveTask  "receive task" waits for a @ref BPMNOS::Execution::MessageDeliveryEvent "message delivery". When the message is delivered, the @ref BPMNOS::Model::Content "message content" is used to update the @ref BPMNOS::Execution::Token::status "status" of the token. Thereafter, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied and  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

A token at a @ref BPMN::DecisionTask  "decision task" waits for a @ref BPMNOS::Execution::ChoiceEvent "choice" to be made. When the choice is made, the @ref BPMNOS::Execution::Token::status "status" of the token is updated accordingly. Thereafter, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied and  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

If any other task increments the timestamp, the token waits for a @ref BPMNOS::Execution::CompletionEvent "completion event" before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is directly updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

@attention Operators for @ref BPMN::SendTask "send tasks", @ref BPMN::ReceiveTask  "receive tasks", and  @ref BPMNOS::Model::DecisionTask "decision tasks" must be instantaneous, i.e., they must not change the timestamp. 
@par
@attention Operators for @ref BPMN::ReceiveTask  "receive tasks" are applied **after** the message is received.
@par
@attention Operators for @ref BPMNOS::Model::DecisionTask "decision tasks" are applied **after** the choices have been made.
@par
@note The timestamp of a task in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state may be in the future, representing an expected completion time.

## COMPLETED

When a token at a task with a @ref BPMN::CompensateBoundaryEvent "compensate boundary event" reaches  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state, a token is created at this boundary event.

A token in  @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state waits for an @ref BPMNOS::Execution::ExitEvent "exit event" indicating that a decision is made to leave the activity. 
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::EXITING "EXITING".



## EXITING
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

Otherwise, the task has been executed successfully and all tokens at boundary events (except @ref BPMN::CompensateBoundaryEvent "compensate boundary events") of the task are withdrawn.

If the task has an outgoing sequence flow, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".
In case of @ref XML::bpmn::tStandardLoopCharacteristics  "loop activities", the token may advance to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state after the respective entry decision has been made for the next loop of the activity.

## DEPARTED

When the token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

## DONE

When the token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

## FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


