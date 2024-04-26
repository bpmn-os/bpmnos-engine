# Intermediate catching events
@page token_flow_logic_intermediate_catching_events Intermediate catching events

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
