# Controller
@page controller Controller

A @ref BPMNOS::Execution::Controller "controller" is responsible for making @ref BPMNOS::Execution::Decision "decisions", i.e.,
- @ref BPMNOS::Execution::EntryDecision "entry decisions",
- @ref BPMNOS::Execution::ChoiceDecision "choice decisions", and
- @ref BPMNOS::Execution::MessageDeliveryDecision "message delivery decisions"

during process execution.

A controller has to @ref BPMNOS::Execution::Controller::connect "connect" to an @ref BPMNOS::Execution::Engine "execution engine" to be informed about relevant changes in the @ref BPMNOS::Execution::SystemState "system state", i.e., about
- @ref BPMNOS::Execution::DecisionRequest "requests for a decision" to
  - @ref BPMNOS::Execution::Observable::Type::EntryRequest  "enter a node",
  - @ref BPMNOS::Execution::Observable::Type::ChoiceRequest "make a choice", or to
  - @ref BPMNOS::Execution::Observable::Type::MessageDeliveryRequest"deliver a message",
- @ref BPMNOS::Execution::DataUpdate "updates of data attribute values or global attribute values",
- @ref BPMNOS::Execution::SequentialPerformerUpdate "sequential performers becoming busy or idle", and
- @ref BPMNOS::Execution::Message "messages sent".

Moreover, by connecting to the execution engine, the engine is enabled to @ref BPMNOS::Execution::EventListener::fetchEvent "fetch events" which are @ref BPMNOS::Execution::EventDispatcher::dispatchEvent "dispatched" by the controller.

## Greedy controller

A controller takes only decisions: the entries of children of sequential ad-hoc subprocesses, the choices at decision tasks, and the deliveries of messages that are not directly addressed. The engine itself enters any other activity as soon as its entry restrictions hold, through its @ref BPMNOS::Execution::EntryObserver "entry observer", and delivers a message whose sender explicitly addresses its recipient or whose recipient explicitly refers to its sender, through its @ref BPMNOS::Execution::DirectMessageObserver "direct message observer", consulting both before the controller. Neither is a decision, so that a controller receives no entry request of an activity outside a sequential ad-hoc subprocess. The @ref BPMNOS::Execution::InstantEntry "instant entry dispatcher", which enters the first child awaiting entry without checking feasibility, and the @ref BPMNOS::Execution::FirstFeasibleEntry "first feasible entry dispatcher", which enters the first feasible child without ranking the candidates by their reward, are available as policies for these entries.

The @ref BPMNOS::Execution::GreedyController "greedy controller" is a @ref BPMNOS::Execution::Controller "controller" that holds a list of dispatchers that are consulted in the given order:

- the @ref BPMNOS::Execution::FirstChoice "first choice dispatcher" evaluates choices for decision tasks and as soon as feasible choices are found for a decision task, it dispatches the best evaluated choices for that task. A choice allowing at least `config.bisect` values is searched by bisection and every other choice is enumerated; if `config.bisect` is not set, every choice with a step is enumerated and a choice of a decimal attribute without a step is searched by bisection,
- the @ref BPMNOS::Execution::CompetingCandidates "competing candidates dispatcher" evaluates message delivery decisions and entry decisions for activities within sequential ad-hoc subprocesses. It dispatches the best evaluated decision among all.

All evaluations are conducted by an @ref BPMNOS::Execution::Evaluator "evaluator".

## Playback controller

The @ref BPMNOS::Execution::PlaybackController "playback controller" plays back the decisions of a log. The log is the JSON output of the engine, an array of records each tagged by its kind, as `bpmnos-greedy` writes it with `--json`, of which the controller reads the clock ticks and the decisions and ignores every other record. A decision is a record tagged `event` whose object states its kind by the key `decision`:

- `{"event": {"decision": "entry", "processId", "instanceId", "nodeId"}}` is the entry of a child of a sequential ad-hoc subprocess by the token of the instance at the node,
- `{"event": {"decision": "choice", "processId", "instanceId", "nodeId", "choices"}}` states the values chosen at a decision task by attribute name, and
- `{"event": {"decision": "messagedelivery", "processId", "instanceId", "nodeId", "message"}}` is the delivery of the message whose origin and, if stated in its header, sender are given.

The reward a recorded decision carries is not read. A decision belongs to the time of the last clock tick `{"event": {"event": "clocktick", "time"}}` before it, and a decision before the first clock tick to the time the run starts. Since the engine carries out everything that is not a decision itself, a log recorded by the engine and a log stating its decisions alone are played back alike.

A decision is dispatched when its request is pending. A decision with a time is due once the run has reached that time, and a decision without a time, stated before the first clock tick, is due from the start of the run, so that a log without clock ticks states only the order of the decisions. At every fetch, the controller dispatches the first due decision, in the order of time and within it of the log, whose request is pending. A decision dispatched later than its time is reported as a deviation; the engine advances time one clock tick at a time, so that in particular a decision whose time lies between two clock ticks is dispatched at the first clock tick after it at the earliest. A decision whose request never becomes pending is not dispatched, so that the run continues until it ends otherwise, at the latest at its end time, and the playback is incomplete. A request is matched by the process, the instance and the node of its token, and a message delivery additionally by a message among the candidates of the request whose origin, header and content agree with those the log states, a value the log leaves undefined agreeing with any. An entry waits while an entry listed before it for the same performer has not been dispatched, so that the entries of every performer keep the order of the log; the performer is the performer node of its sequential ad-hoc subprocess within the instance of the entry, without the event subprocesses and multi-instance activities within the performer by which the instance is disambiguated. Every other decision is dispatched as soon as its request is pending. Before it is dispatched, a decision is evaluated by the evaluator given to the controller, a @ref BPMNOS::Execution::LocalEvaluator "local evaluator" by default, which records its reward in the log of the playback. A decision the evaluator finds infeasible is not dispatched but waits until it becomes feasible, as the decision of any controller has to.

The playback stops at the first mismatch, by a termination event, since the objective of a run deviating from the log validates nothing. A mismatch is a choice lacking the value of one of the attributes of the decision task, or the entry of an activity outside a sequential ad-hoc subprocess, which is not a decision. The controller reports the number of decisions read and dispatched, the deviations and the mismatch, which `bpmnos-playback` appends to its JSON output as a record tagged `playback`.

A log can be played back exactly for a model whose executions are determined by the times of their decisions, the order of the entries of every performer, the values chosen and the messages delivered, that is for a model with deterministic data, a static or expected-value data provider, whose decisions are identified by process, instance and node, and whose times lie on the clock grid of the engine. Assignment, scheduling and routing problems belong to this class. For such a model, a formulation of its decisions as a mixed-integer program can be checked by playing back a solution: the objective of the playback must equal the objective of the solution, and the first decision that cannot be played back shows where formulation and model disagree.

## Evaluator

The @ref BPMNOS::Execution::Evaluator "evaluator" is responsible for determining whether a decision can be taken without violating any restrictions and to quantify the value of taking the decision.

### Local evaluator
The @ref BPMNOS::Execution::LocalEvaluator "Local evaluator" makes local observations to evaluate a decision.

### Guided evaluator
The @ref BPMNOS::Execution::GuidedEvaluator "Guided evaluator" makes local observations and includes @ref BPMNOS::Model::Guidance "guidance" provided with the model to evaluate a decision. The attributes a guidance declares take only the values the model assigns to them, by expression or by the operators of the guidance, and a data provider rejects a value given for them in the data. An entry guidance may be given only for a child of a sequential ad-hoc subprocess, whose entry is decided by its performer; any other activity is entered as soon as its restrictions are satisfied, so that the model reader rejects an entry guidance given for it.
