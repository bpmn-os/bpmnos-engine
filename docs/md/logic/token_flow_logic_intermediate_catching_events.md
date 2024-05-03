# Intermediate catching events
@page token_flow_logic_intermediate_catching_events Intermediate catching events

The state of a token at an intermediate catching event is immediately advanced from @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED", to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY" and awaits the trigger.
If the token is at a @BPMN::MessageBoundaryEvent "message boundary event", the @ref BPMNOS::Model::Content "message content" is used to update the @ref BPMNOS::Execution::Token::status "status" of the token.

After the event is triggered, the state is advanced @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".
Then, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".


<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>
