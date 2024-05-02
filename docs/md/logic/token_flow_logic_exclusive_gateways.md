# Exclusive gateways
@page token_flow_logic_exclusive_gateways Exclusive gateways

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

@todo

## States

<pre class="mermaid">
stateDiagram-v2
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> DEPARTED
    DEPARTED --> [*]
</pre>



