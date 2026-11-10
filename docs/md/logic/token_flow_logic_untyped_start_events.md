# Untyped start events
@page token_flow_logic_untyped_start_events Untyped start events

A token at an untyped start event is created in @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state, and its state is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
After entry, a token is created for the @ref BPMN::TypedStartEvent "start event" of each @ref BPMN::EventSubProcess "event-subprocess" within the same @ref BPMN::Scope "scope".
The state is then advanced to @ref BPMNOS::Execution::Token::State::BUSY "BUSY", an untyped start event awaiting no trigger, and on to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

Upon completion, the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the start event are applied.

After completion, the start event of a process checks the entry restrictions of the process, whereas the start event of a subprocess, which has checked its entry restrictions on entry, checks its own completion restrictions, which include the full restrictions of the subprocess it inherits.
If the restrictions are violated, the state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state feasibility <<choice>>
    state departure <<choice>>
    [*] --> CREATED
    CREATED --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED
    COMPLETED --> feasibility
    feasibility --> departure: [feasible]
    feasibility --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>

@note The operators of a start event must be instantaneous, i.e. they must not change the timestamp.
