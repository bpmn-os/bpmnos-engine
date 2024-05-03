# Event-based gateways
@page token_flow_logic_eventbased_gateways Event-based gateways

A token arrivaing at an event-based gateway receives @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state.
The state is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" and then to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".
A copy of the token is created for each outgoing sequence flow and the original token.

## States

<pre class="mermaid">
stateDiagram-v2
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> BUSY
    BUSY --> [*]
</pre>

 
The state of each token copy is immediately advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED".
<pre class="mermaid">
stateDiagram-v2
    [*] --> DEPARTED
    DEPARTED --> [*]
</pre>

When any of the subsequent catch event is triggered the respective token advances, all other token copies are withdrawn, and the token at the event-based gateway is disposed. 

