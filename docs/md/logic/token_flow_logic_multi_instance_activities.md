@page token_flow_logic_multi_instance_activities Multi-instance activities

A token at an activity with a multi-instance marker changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED/CREATED
    ARRIVED/CREATED --> WAITING: ready event
    WAITING --> departure
    WAITING --> FAILING
    WAITING --> FAILED
    FAILING --> FAILED
    departure --> DEPARTED: [outgoing sequence flow]
    departure --> DONE: [no outgoing sequence flow]
    DEPARTED --> [*]
    DONE --> [*]
    FAILED --> [*]
</pre>


## ARRIVED / CREATED

A token enters @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state when it arrives via an incoming sequence flow, or @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state when the activity has no incoming sequence flows (e.g., activities in ad-hoc subprocesses).

In either state, the token waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs, the token state is updated to @ref BPMNOS::Execution::Token::State::WAITING "WAITING" after token copies for each activity instance are created as described below.

If a @ref BPMNOS::Model::ExtensionElements::loopCardinality "loop cardinalty parameter" is provided, the respective number of token copies is created.
If the token is at a multi-instance @ref BPMN::SendTask "send task" or @ref BPMN::ReceiveTask  "receive task", a token copy is created for each @ref BPMNOS::Model::ExtensionElements::messageDefinitions "message definition".

If a @ref BPMNOS::Model::ExtensionElements::loopIndex "loop index parameter" is provided, the respective attribute of the *i* th token copy receives the value *i*.

Depending on whether the multi-instance activity is parallel or sequential, each token copy is advanced in parallel or sequential fashion.
Each token copy begins in the @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state when it is created from the main token; no ready event of its own is raised, as the single ready event is consumed by the main token. It then changes its states as described in @ref token_flow_logic_subprocesses or @ref token_flow_logic_tasks, except that the @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" and @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" states are skipped (a copy has neither an incoming nor an outgoing sequence flow). Upon exiting, each copy advances to the @ref BPMNOS::Execution::Token::State::DONE "DONE" state.

For a sequential multi-instance activity, all copies are created in the @ref BPMNOS::Execution::Token::State::CREATED "CREATED" state, but only the first copy advances to the @ref BPMNOS::Execution::Token::State::READY "READY" state immediately; each subsequent copy advances from @ref BPMNOS::Execution::Token::State::CREATED "CREATED" to @ref BPMNOS::Execution::Token::State::READY "READY" only once its predecessor has exited. For a parallel multi-instance activity, all copies advance to the @ref BPMNOS::Execution::Token::State::READY "READY" state at once.



## WAITING

A token in @ref BPMNOS::Execution::Token::State::WAITING "WAITING" state remains in this state until all token copies have reached the  @ref  BPMNOS::Execution::Token::State::DONE "DONE" state or have terminated with a failure.
The token state is then updated accordingly.

## DEPARTED

When the token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

## DONE

When the token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

## FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


