# Typed start events
@page token_flow_logic_typed_start_events Typed start events

The state of a token at a typed start event is immediately advanced from @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY" and awaits the trigger.
If the token is at a @ref BPMN::MessageStartEvent "message start event", the @ref BPMNOS::Model::Content "message content" is used to update the @ref BPMNOS::Execution::Token::status "status" of the token. Thereafter, the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the event-subprocess are applied.

After the start event is triggered, the state is advanced @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".
If the respective @ref BPMN:EventSubProcess "event-subprocesses" is interrupting, all other tokens within the scope of the event-subrocess are withdrawn.
Otherwise, a new token is created allowing the event-subprocess to be triggered again.

After completion, the entry scope restrictions of the event-subprocess are checked.
If the restrictions are violated, the state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, state is changed to @ref BPMNOS::Execution::Token::State::EXITING "EXITING".
Then, the full scope restrictions of the context are validated and either a failure is raised or the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    state departure <<choice>>
    [*] --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> feasibleEntry
    feasibleEntry --> EXITING: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    EXITING --> feasibleExit
    feasibleExit --> departure: [feasible]
    feasibleExit --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>





@attention @ref BPMN::TypedStartEvent "Typed start events" are only supported for @ref BPMN:EventSubProcess "event-subprocesses".
@note Operators for event-subprocesses must be instantaneous, i.e. they must not change the timestamp.
