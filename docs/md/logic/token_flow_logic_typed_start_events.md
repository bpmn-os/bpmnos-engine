# Typed start events
@page token_flow_logic_typed_start_events Typed start events

A token at a typed start event is generated when the trigger occurs, and its state advances from @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" through @ref BPMNOS::Execution::Token::State::BUSY "BUSY" to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" instantaneously.
The @ref BPMNOS::Model::Content "content" of the message or signal that triggered it is part of the @ref BPMNOS::Execution::Token::status "status" the token is generated with.

Upon completion, the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the scope the start event belongs to are applied.
If the start event belongs to an interrupting @ref BPMN::EventSubProcess "event-subprocess", all other tokens within the scope of the event-subprocess are withdrawn.
If it belongs to a non-interrupting event-subprocess, the event-subprocess may be triggered again.
If it belongs to a @ref BPMN::Process "process", each trigger creates an instance of that process.

After completion, the entry scope restrictions of the scope are checked.
If the restrictions are violated, the state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state departure <<choice>>
    [*] --> ENTERED: trigger
    ENTERED --> BUSY
    BUSY --> COMPLETED
    COMPLETED --> feasibleEntry
    feasibleEntry --> departure: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>


@attention @ref BPMN::TypedStartEvent "Typed start events" are supported for @ref BPMN::EventSubProcess "event-subprocesses" and, restricted to @ref BPMN::MessageStartEvent "message start events" and @ref BPMN::SignalStartEvent "signal start events", for @ref BPMN::Process "processes".
@note Operators for elements with a scope must be instantaneous, i.e. they must not change the timestamp.
