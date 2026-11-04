# Intermediate catching events
@page token_flow_logic_intermediate_catching_events Intermediate catching events

The state of a token at an intermediate catching event is immediately advanced from @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED", to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY" and awaits the trigger.
If the token is at a @ref BPMN::MessageBoundaryEvent "message boundary event", the content object of the message is held by the token until the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the event read it.

After the event is triggered, the state is advanced @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED", and the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the event are applied.
If the status they produce violates any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions", the token state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".
Otherwise, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".


<pre class="mermaid">
stateDiagram-v2
    state feasibility <<choice>>
    state departure <<choice>>
    [*] --> ARRIVED
    ARRIVED --> ENTERED
    ENTERED --> BUSY
    BUSY --> COMPLETED: trigger
    COMPLETED --> feasibility
    feasibility --> departure: [feasible]
    feasibility --> FAILED: [infeasible]
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>
