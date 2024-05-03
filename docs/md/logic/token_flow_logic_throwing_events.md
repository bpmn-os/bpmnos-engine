# Throwing events
@page token_flow_logic_throwing_events Throwing events

The state of a token at an throwing event is immediately advanced from @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED", to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
Then, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

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

@note For @ref BPMN::SendTask "send tasks", the token flow logic described in @ref token_flow_logic_tasks applies.


