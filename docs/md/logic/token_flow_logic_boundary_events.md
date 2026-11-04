# Boundary events
@page token_flow_logic_boundary_events Boundary events

A token at a boundary event is created in @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state, and its state is immediately advanced through @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY", where it awaits the trigger.
If the token is at a @ref BPMN::MessageBoundaryEvent "message boundary event", the content object of the message is held by the token until the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the event read it.

After the boundary event is triggered, the state is advanced @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED", and the @ref BPMNOS::Model::ExtensionElements::operators "operators" of the event are applied.
If the status they produce violates any of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions", the token state is changed to @ref BPMNOS::Execution::Token::State::FAILED "FAILED", and the activity is neither interrupted nor is a new token created at the boundary event.
Otherwise, if the boundary event is interrupting, all other tokens within the scope of the respective acitvity are withdrawn.
Otherwise, a new token is created at the boundary event allowing it to be triggered again.

Then, the token state is changed to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state feasibility <<choice>>
    state departure <<choice>>
    [*] --> CREATED
    CREATED --> ENTERED
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
