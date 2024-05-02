# Boundary events
@page token_flow_logic_boundary_events Boundary events

## States

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>
