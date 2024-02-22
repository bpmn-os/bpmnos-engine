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



