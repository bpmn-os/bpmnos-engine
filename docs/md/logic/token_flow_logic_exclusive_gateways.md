# Exclusive gateways
@page token_flow_logic_exclusive_gateways Exclusive gateways

A token at an exclusive gateway changes its state as follows:


<pre class="mermaid">
stateDiagram-v2
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> DEPARTED
    DEPARTED --> [*]
</pre>

If the gateway has more than one outgoing sequence flow, the token departs along the first one whose condition is satisfied, and along the default flow if none is. If no condition is satisfied and the gateway has no default flow, the model is in error, and the run is aborted with an exception.



