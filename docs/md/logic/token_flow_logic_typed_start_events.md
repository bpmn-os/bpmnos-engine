# Typed start events
@page token_flow_logic_typed_start_events Typed start events

@todo

## States

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

When a typed start event is trigerred, the relevant status and data of the token is updated if necessary and the state is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".
Then, the entry scope restrictions of the context are validated and either a failure is raised or the (instantaneous) operators defined for the context are applied and state is changed to @ref BPMNOS::Execution::Token::State::EXITING "EXITING".
Then, the full scope restrictions of the context are validated and either a failure is raised or the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".
