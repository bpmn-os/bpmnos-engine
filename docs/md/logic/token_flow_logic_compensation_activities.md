@page token_flow_logic_compensation_activities Compensation activities

When an activity with a @ref BPMN::CompensateBoundaryEvent "compensate boundary event" completes, a token is created at the boundary event holding the status the activity completed with. Its state advances from @ref BPMNOS::Execution::Token::State::CREATED "CREATED" through @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" to @ref BPMNOS::Execution::Token::State::BUSY "BUSY", and it waits there until compensation is triggered. When compensation is triggered, the boundary event completes, its operators being applied, and the token arrives at the compensation activity with the status of the enclosing scopes the compensated activity completed with, receiving @ref BPMNOS::Execution::Token::State::ARRIVED "ARRIVED" state. The data provider then gives the ready event as for any activity: the instance of the compensation activity comes into being with the status the token holds, the data of the scope at that time and the time of the trigger as its timestamp, its own attributes taking the values the model or the instance data give them, and, for the stochastic data provider, its ready values. A compensation activity becomes ready when compensation is triggered, so that the data may give it neither a disclosure of its own nor a ready value of its timestamp. The token receives @ref BPMNOS::Execution::Token::State::READY "READY" state and enters the compensation activity without an entry decision, receiving @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state. Each token compensates one completed instance of the compensated activity, also of a multi-instance activity. The token changes its state according to the reduced state diagram below and as described in @ref token_flow_logic_subprocesses or @ref token_flow_logic_tasks. Feasibility is checked directly after @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state without waiting for an exit decision. When a token has feasibly completed, the token is disposed without further.
Otherwise, a failure is raised.
 
<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleCompletion <<choice>>
    [*] --> ARRIVED
    ARRIVED --> READY: ready event
    READY --> ENTERED
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED
    BUSY --> FAILED: failure
    COMPLETED --> feasibleCompletion
    feasibleCompletion --> [*]: [feasible]
    feasibleCompletion --> FAILED: [infeasible]
    FAILED --> [*]
</pre>


@note A compensation activity can not be compensated.
