# Inclusive gateways
@page token_flow_logic_inclusive_gateways Inclusive gateways

A token arriving at an inclusive gateway receives @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state.
If multiple sequence flows enter the gateway, the state is updated to @ref BPMNOS::Execution::Token::State::WAITING "WAITING", indicating that the token may have to be merged with other tokens.
Otherwise, the state is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

<pre class="mermaid">
stateDiagram-v2
    state merge <<choice>>
    [*] --> ARRIVED
    ARRIVED --> merge
    merge --> ENTERED: [one incoming sequence flow]
    merge --> WAITING: [multiple incoming sequence flows]
    WAITING --> ENTERED: unmerged release
    WAITING --> [*]: merging
    ENTERED --> [*]
</pre>

A converging inclusive gateway is activated when at least one token waits at it and no other token of the same scope is at a predecessor of the gateway, a predecessor being a flow node from which an incoming sequence flow of the gateway can be reached along sequence flows without visiting the gateway (@ref BPMNOS::Model::Model::predecessors). Tokens that have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and tokens at boundary events are on no sequence flow and are disregarded, and a subprocess is represented by the token at it. Whether the gateway is activated is determined whenever all tokens have been advanced as far as possible. Upon activation, a token is consumed from each incoming sequence flow on which a token waits, further tokens on the same sequence flow waiting for the next activation. If a single token is consumed, it is released without being merged and its state is advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED". Otherwise, a new token receives the merged status of the consumed tokens, as at a @ref token_flow_logic_parallel_gateways "parallel gateway", the consumed tokens are disposed, and the state of the merged token is advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".

@attention The rule coincides with the activation rule of the BPMN specification for acyclic models in which a path carries at most one token at a time. If a further token can travel a path that has already delivered a token to the gateway, the gateway also waits for that token, whereas the specification activates it at once and once more when the token arrives.

If the gateway has more than one outgoing sequence flow, a copy of the token that has entered the gateway is created for each outgoing sequence flow whose condition is satisfied, and for the default flow if no condition is satisfied, and the state of each copy is advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED". The token that has entered the gateway is disposed. If no condition is satisfied and the gateway has no default flow, the model is in error, and the run is aborted with an exception. If the gateway has one outgoing sequence flow, the state of the token is advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
