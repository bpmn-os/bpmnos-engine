@page token_flow_logic_multi_instance_activities Multi-instance activities

The token flow logic for activities depends on whether the multi-instance marker for the activity is set or not. 

# Multi-instance activities

## States

<pre class="mermaid">
stateDiagram-v2
    state departure <<choice>>
    [*] --> ARRIVED
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

<pre class="mermaid">
stateDiagram-v2
    state feasibleEntry <<choice>>
    state feasibleExit <<choice>>
    state failure <<choice>>
    [*] --> READY
    READY --> ENTERED: entry event
    ENTERED --> feasibleEntry
    feasibleEntry --> BUSY: [feasible]
    feasibleEntry --> FAILED: [infeasible]
    BUSY --> COMPLETED
    BUSY --> failure: failure
    COMPLETED --> EXITING: exit event
    EXITING --> feasibleExit
    feasibleExit --> [*]: [feasible]
    feasibleExit --> failure: [infeasible]
    failure --> FAILING: [(ad-hoc) subprocess]
    failure --> FAILED: [task]
    FAILING --> FAILED
    FAILED --> [*]
</pre>

