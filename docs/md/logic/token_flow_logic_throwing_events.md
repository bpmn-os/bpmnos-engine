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
@note A token at a @ref BPMN::SignalThrowEvent "signal throw event" departs without waiting for anything, a signal being broadcast rather than delivered to a recipient. The signal it raises is announced and then delivered to the tokens waiting for it, which are those waiting when the broadcast is made rather than at the instant the throwing token departed, so a token that was itself on its way to a @ref BPMN::SignalCatchEvent "signal catch event" still receives it.


