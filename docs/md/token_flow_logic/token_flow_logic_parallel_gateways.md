# Parallel gateways
@page token_flow_logic_parallel_gateways Parallel gateways

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

## States

<pre class="mermaid">
stateDiagram-v2
    state merge <<choice>>
    state split <<choice>>
    [*] --> ARRIVED
    ARRIVED --> merge
    merge --> ENTERED: [one incoming sequence flow]
    merge --> WAITING
    WAITING --> [*]: [multiple incoming sequence flows]
    ENTERED --> split
    split --> [*]: [multiple outgoing sequence flows]
    split --> DEPARTED: [one outgoing sequence flow]
</pre>

<pre class="mermaid">
stateDiagram-v2
    state split <<choice>>
    [*] --> ENTERED
    ENTERED --> split
    split --> [*]: [multiple outgoing sequence flows]
    split --> DEPARTED: [one outgoing sequence flow]
</pre>

<pre class="mermaid">
stateDiagram-v2
    [*] --> DEPARTED
    DEPARTED --> [*]
</pre>


