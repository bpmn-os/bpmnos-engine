# Event-based gateways
@page token_flow_logic_eventbased_gateways Event-based gateways

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

## States

<pre class="mermaid">
stateDiagram-v2
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> BUSY
    BUSY --> [*]
</pre>

<pre class="mermaid">
stateDiagram-v2
    [*] --> DEPARTED
    DEPARTED --> [*]
</pre>


