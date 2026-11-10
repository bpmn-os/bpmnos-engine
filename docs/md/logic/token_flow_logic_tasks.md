# Tasks
@page token_flow_logic_tasks Tasks

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not and whether the activity is for compensation or not.

- @ref token_flow_logic_multi_instance_activities "Multi-instance activities"
- @ref token_flow_logic_compensation_activities "Compensation activities"

# Tasks (excluding multi-instance and compensation activities)

A token at a task changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleCompletion <<choice>>
    state departure <<choice>>
    [*] --> ARRIVED/CREATED
    note left of ARRIVED/CREATED
      For multi-instance activity copies, the ARRIVED state
      is skipped; the copy starts in CREATED and advances to
      READY without a ready event
    end note
    ARRIVED/CREATED --> READY: ready event
    READY --> ENTERED: entry event
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED: completion event
    BUSY --> FAILED: failure
    COMPLETED --> feasibleCompletion
    feasibleCompletion --> departure: [feasible]
    note left of departure
      In case of loop activities, the token may return to READY state
      and wait for the respective entry event.
    end note
    feasibleCompletion --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>


## ARRIVED / CREATED

A token enters @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state when it arrives via an incoming sequence flow, or @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state when the activity has no incoming sequence flows (e.g., activities in ad-hoc subprocesses).

In either state, the token waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs the token state is updated to @ref BPMNOS::Execution::Token::State::READY "READY".


## READY

A token in  @ref BPMNOS::Execution::Token::State::READY "READY" state waits for an @ref BPMNOS::Execution::EntryEvent "entry event". For a child of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess", the entry event indicates that its performer has decided to start with the activity. Any other activity is entered as soon as its entry restrictions hold on the status the token has with the current time as its timestamp, as a conditional event is triggered as soon as its condition holds; the engine then dispatches the entry event itself, and the token waits as long as the restrictions do not hold, until the end of the run if they never hold.
When the event occurs the token state is updated to  @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

## ENTERED

Upon entry, the initial assignment of @ref BPMNOS::Model::Attribute "attributes" are conducted in the order of attribute definitions.
Then, a token is created at each @ref BPMN::BoundaryEvent "boundary event" (excluding @ref BPMN::CompensateBoundaryEvent "compensate boundary events").
These tokens inherit the @ref BPMNOS::Execution::Token::status "status attributes" of the task token. The tokens at boundary events are created only if they do not exist yet, so that the re-entries of a @ref XML::bpmn::tStandardLoopCharacteristics "standard loop" keep the tokens created on the first entry, and for a multi-instance activity, whose tokens at boundary events belong to the main token, only the first instance to enter creates them.
Then, feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".


## BUSY

For a task which is not a @ref BPMN::SendTask  "send task", @ref BPMN::ReceiveTask  "receive task" or @ref BPMNOS::Model::DecisionTask "decision task",
 the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the token.
 
If the token resides at a @ref BPMN::SendTask "send task", the operators are applied and a @ref BPMNOS::Execution::Message "message" is created. The token waits for a @ref BPMNOS::Execution::MessageDeliveryEvent "message delivery event" at any of the recipients before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

A token at a @ref BPMN::ReceiveTask  "receive task" waits for a @ref BPMNOS::Execution::MessageDeliveryEvent "message delivery". When the message is delivered, the token holds the content object of the message. Thereafter, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied, reading the content object, and  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

A token at a @ref BPMNOS::Model::DecisionTask  "decision task" waits for a @ref BPMNOS::Execution::ChoiceEvent "choice" to be made. When the choice is made, the chosen values are assigned to their attributes, which may be @ref BPMNOS::Execution::Token::status "status", data or global attributes, as for operators. Data and global attributes written by the choices or the operators are reported by a @ref BPMNOS::Execution::DataUpdate "data update" once the operators have been applied, also if the decision task has no operators, so that conditional events and pending decisions depending on them are reconsidered. Thereafter, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied and  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

For any other task, the token waits for a @ref BPMNOS::Execution::CompletionEvent "completion event" before the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

@attention Operators for @ref BPMN::SendTask "send tasks", @ref BPMN::ReceiveTask  "receive tasks", and  @ref BPMNOS::Model::DecisionTask "decision tasks" must be instantaneous, i.e., they must not change the timestamp. For @ref BPMN::SendTask "send tasks" operatores are applied **before** the message is sent, for @ref BPMN::ReceiveTask  "receive tasks" they are applied **after** the message is received, and for @ref BPMNOS::Model::DecisionTask "decision tasks" they are applied **after** the choices have been made, respectively.
@par
@note The timestamp of a task in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state may be in the future, representing an expected completion time.

## COMPLETED

After completion, feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated,  the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

Otherwise, the task has been executed successfully. If the task is a child of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess", its performer is released, and all tokens at boundary events (except @ref BPMN::CompensateBoundaryEvent "compensate boundary events") of the task are withdrawn. If the task has a @ref BPMN::CompensateBoundaryEvent "compensate boundary event", a token is created at this boundary event once the objective has been updated, so that a task failing its restrictions is not compensated.

If the task has an outgoing sequence flow, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".
In case of @ref XML::bpmn::tStandardLoopCharacteristics  "loop activities", the @ref BPMNOS::Execution::Token::state "token state" is instead updated to @ref BPMNOS::Execution::Token::State::READY "READY" if a further loop of the activity is to be performed, and the token waits for the respective entry event.

## DEPARTED

When the token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

## DONE

When the token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

## FAILED

If the token resides at a child of a @ref BPMNOS::Model::SequentialAdHocSubProcess "sequential ad-hoc subprocess" and holds its performer, the performer is released when the token fails, so that another child may be entered. The same applies when the token is withdrawn.

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


