# Throwing events
@page token_flow_logic_throwing_events Throwing events

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

@todo

## States

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>



