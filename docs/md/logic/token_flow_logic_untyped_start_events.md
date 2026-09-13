# Untyped start events
@page token_flow_logic_untyped_start_events Untyped start events

The state of a token at an untyped start event is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
After entry, a token is created for the @ref BPMN::TypedStartEvent "start event" of each @ref BPMN::EventSubProcess "event-subprocess" within the same @ref BPMN::Scope "scope".
The state is then advanced to @ref BPMNOS::Execution::Token::State::BUSY "BUSY", an untyped start event awaiting no trigger, and on to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

Upon completion, the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the scope the start event belongs to are applied.

After completion, the entry scope restrictions of the scope are checked.
If the restrictions are violated, the state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state departure <<choice>>
    [*] --> ENTERED
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

@note Operators for elements with a scope must be instantaneous, i.e. they must not change the timestamp.
