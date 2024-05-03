# Boundary events
@page token_flow_logic_boundary_events Boundary events

The state of a token at a boundary event is immediately advanced from @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY" and awaits the trigger.
If the token is at a @BPMN::MessageBoundaryEvent "message boundary event", the @ref BPMNOS::Model::Content "message content" is used to update the @ref BPMNOS::Execution::Token::status "status" of the token.

After the boundary event is triggered, the state is advanced @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".
If the boundary event is interrupting, all other tokens within the scope of the respective acitvity are withdrawn.
Otherwise, a new token is created at the boundary event allowing it to be triggered again.

Then, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>
