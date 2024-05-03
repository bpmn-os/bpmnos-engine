@page token_flow_logic_multi_instance_activities Multi-instance activities

A token at an activity with a multi-instance marker changes its state as follows:

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED
    note left of ARRIVED
      If an activity does not have any incoming sequence flows,
      the @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state is skipped 
    end note
    ARRIVED --> WAITING: ready event
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


## ARRIVED

A token in  @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state waits for a @ref BPMNOS::Execution::ReadyEvent "ready event" indicating that all relevant data has become known. When the event occurs, the token state is updated to  @ref BPMNOS::Execution::Token::State::WAITING "WAITING" after token copies for each activity instance are created as described below.

If a @ref BPMNOS::Model::ExtensionElements::loopCardinality "loop cardinalty parameter" is provided, the respective number of token copies is created.
If the token is at a multi-instance @ref BPMN::SendTask "send task" or @ref BPMN::ReceiveTask  "receive task", a token copy is created for each @ref BPMNOS::Model::ExtensionElements::messageDefinitions "message definition".

If a @ref BPMNOS::Model::ExtensionElements::loopIndex "loop index parameter" is provided, the respective attribute of the *i* th token copy receives the value *i*.
Moreover, for each @ref BPMNOS::Model::Attribute "attribute" for which a @ref BPMNOS::Model::Attribute::collection "collection" is provided, the *i* th token copy receives the value of the *i* th element in the collection.

Depending on whether the multi-instance activity is parallel or sequential, each token copy is advanced in parallel or sequential fashion.
These tokens change their states as described in @ref token_flow_logic_subprocesses or @ref token_flow_logic_tasks except that the @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED", @ref  BPMNOS::Execution::Token::State::DEPARTED "DEPARTED", and @ref BPMNOS::Execution::Token::State::DONE "DONE" states are skipped.



## WAITING

A token in @ref BPMNOS::Execution::Token::State::WAITING "WAITING" state remains in this state until all token copies have passed the  @ref  BPMNOS::Execution::Token::State::EXITING "EXITING" state or have terminated with a failure.
The token state is then updated accordingly.

## DEPARTED

When the token reaches @ref BPMNOS::Execution::Token::State::DEPARTED "DEPARTED" state, the token moves to the next node.

## DONE

When the token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the token remains at the node until all other tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state or a failure occurred.

@note When all tokens in the parent scope have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state, the state of the parent token is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state.

## FAILED

Upon failure, the state of a token at an @ref BPMN::ErrorBoundaryEvent "error boundary event" is changed to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED". If no such @ref BPMN::ErrorBoundaryEvent "error boundary event" exists, the error is bubbled up to its parent scope.


