# Untyped start events
@page token_flow_logic_untyped_start_events Untyped start events

The state of a token at an untyped start event is immediately advanced to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED".
After entry, a token is created for the @ref BPMN::Model::TypedStartEvent "start event" of each @ref BPMN::Model::EventSubProcess "event-subprocess" within the same @ref BPMN::Model::Scope "scope".
Then, The state of the token is advanced to @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" or @ref BPMNOS::Execution::Token::State::DONE "DONE".

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ENTERED
    ENTERED --> departure
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
</pre>

@note Operators are applied and feasibility is checked for the token at the respective (sub)process before the token enters the start event.
