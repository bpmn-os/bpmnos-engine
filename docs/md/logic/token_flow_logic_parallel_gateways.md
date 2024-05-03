# Parallel gateways
@page token_flow_logic_parallel_gateways Parallel gateways

A token arrivaing at a parallel gateway receives @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state.
If multiple sequence flow enter the gateway, the state update to @ref BPMNOS::Execution::Token::State::WAITING "WAITING", indicating that the token has to be merged with other tokens.
Otherwise, the state is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
If only one sequence flow departs the gateway, the state is immediately advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
<pre class="mermaid">
stateDiagram-v2
    state merge <<choice>>
    state split <<choice>>
    [*] --> ARRIVED
    ARRIVED --> merge
    merge --> ENTERED: [one incoming sequence flow]
    merge --> WAITING: [multiple incoming sequence flows]
    WAITING --> [*]
    ENTERED --> split
    split --> [*]: [multiple outgoing sequence flows]
    split --> DEPARTED: [one outgoing sequence flow]
</pre>

When a token has arrived at each incoming sequence flow, a new token is generated that receive the merged status of all waiting tokens which are disposed afterwards.
The merged status contains all attribute values that have the same value or are undefined for all waiting tokens. If waiting tokens have differently defined attribute values the merged token receives an undefined value for this attribute. 
The state of the merged token is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
If only one sequence flow departs the gateway, the state is immediately advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".

<pre class="mermaid">
stateDiagram-v2
    state split <<choice>>
    [*] --> ENTERED
    ENTERED --> split
    split --> [*]: [multiple outgoing sequence flows]
    split --> DEPARTED: [one outgoing sequence flow]
</pre>

If multiple sequence flows depart the gateway, a copy of the token that has entered the gateway is created for each outgoing sequence flow and the state of each copy is immediately advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED". The token that has entered the gateway is disposed.

<pre class="mermaid">
stateDiagram-v2
    [*] --> DEPARTED
    DEPARTED --> [*]
</pre>





