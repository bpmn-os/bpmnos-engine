# Inclusive gateways
@page token_flow_logic_inclusive_gateways Inclusive gateways

A token at a diverging inclusive gateway changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> [*]
</pre>

If the gateway has more than one outgoing sequence flow, a copy of the token that has entered the gateway is created for each outgoing sequence flow whose condition is satisfied, and for the default flow if no condition is satisfied, and the state of each copy is advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED". The token that has entered the gateway is disposed. If no condition is satisfied and the gateway has no default flow, the model is in error, and the run is aborted with an exception. If the gateway has one outgoing sequence flow, the state of the token is advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".

@attention Converging inclusive gateways are **not** supported.
