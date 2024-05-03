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



