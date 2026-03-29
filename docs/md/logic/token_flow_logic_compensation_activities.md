@page token_flow_logic_compensation_activities Compensation activities

When a compensation is triggered, a token at a compensation activity immediatley  enters the respective compensation activity  and receives @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED" state. It changes its state according to the reduced state diagram below and as described in @ref token_flow_logic_subprocesses or @ref token_flow_logic_tasks. Feasibility is checked directly after @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED" state without waiting for an exit decision. When a token has feasibly completed, the token is disposed without further.
Otherwise, a failure is raised.
 
<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleCompletion <<choice>>
    [*] --> ENTERED
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
