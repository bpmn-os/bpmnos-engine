# Untyped start events
@page token_flow_logic_untyped_start_events Untyped start events

The token flow logic for activities depends on whether the start event is typed or not.

## States

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ENTERED
    ENTERED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>
