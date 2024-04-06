# Catching events
@page token_flow_logic_catching_events Catching events

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

## States

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>

@note The @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state is not relevant for @ref BPMN::BoundaryEvents "boundary events" and @ref BPMN::TypedStartEvents "start events" of @ref BPMN::EventSubProcess "event-subprocesses". 
For such events, a token is created when the respective context is activated and the state of this token is set to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state.

