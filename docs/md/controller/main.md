# Controller
@page controller Controller

A @ref BPMNOS::Execution::Controller "controller" is responsible for making @ref BPMNOS::Execution::Decision "decisions", i.e.,
- @ref BPMNOS::Execution::EntryDecision "entry decisions",
- @ref BPMNOS::Execution::ExitDecision "exit decisions",
- @ref BPMNOS::Execution::ChoiceDecision "choice decisions", and
- @ref BPMNOS::Execution::MessageDeliveryDecision "message delivery decisions"

during process execution.

A controller has to @ref BPMNOS::Execution::Controller::connect "connect" to an @ref BPMNOS::Execution::Engine "execution engine" to be informed about relevant changes in the @ref BPMNOS::Execution::SystemState "system state", i.e., about
- @ref BPMNOS::Execution::DecisionRequest "requests for a decision" to
  - @ref BPMNOS::Execution::Observable::Type::EntryRequest  "enter a node",
  - @ref BPMNOS::Execution::Observable::Type::ExitRequest "exit a node",
  - @ref BPMNOS::Execution::Observable::Type::ChoiceRequest "make a choice", or to
  - @ref BPMNOS::Execution::Observable::Type::MessageDeliveryRequest"deliver a message",
- @ref BPMNOS::Execution::DataUpdate "updates of data attribute values or global attribute values",
- @ref BPMNOS::Execution::SequentialPerformerUpdate "sequential performers becoming busy or idle", and
- @ref BPMNOS::Execution::Message "messages sent".

Moreover, by connecting to the execution engine, the engine is enabled to @ref BPMNOS::Execution::EventListener::fetchEvent "fetch events" which are @ref BPMNOS::Execution::EventDispatcher::dispatchEvent "dispatched" by the controller.

## Greedy controller

The @ref BPMNOS::Execution::GreedyController "greedy controller" is a @ref BPMNOS::Execution::Controller "controller" that holds a list of dispatchers that are consulted in the given order:

- the @ref BPMNOS::Execution::FirstFeasibleExit "exit dispatcher" evaluates the pending exit decisions and dispatches the first feasible exit,
- the @ref BPMNOS::Execution::FirstFeasibleEntry "entry dispatcher" evaluates the pending entry decisions (excluding those in a sequential ad-hoc subprocess) and dispatches the first feasible entry,
- the @ref BPMNOS::Execution::InstantDirectMessage "direct message dispatcher" dispatches a message delivery without evaluation whenever the sender explicitly addresses its recipient or the recipient explicitly refers to the sender,
- the @ref BPMNOS::Execution::FirstEnumeratedChoice "enumerated choice dispatcher" (, )or the @ref BPMNOS::Execution::FirstBisectionalChoice "bisectional choice dispatcher" when `config.bisection` is set) evaluates choices for decision tasks and as soon as feasible choices are found for a decision task, it dispatches the best evaluated choices for that task,
- the @ref BPMNOS::Execution::CompetingCandidates "competing candidates dispatcher" evaluates message delivery decisions and entry decisions for activities within sequential ad-hoc subprocesses. It dispatches the best evaluated decision among all.

All evaluations are conducted by an @ref BPMNOS::Execution::Evaluator "evaluator".

## Evaluator

The @ref BPMNOS::Execution::Evaluator "evaluator" is responsible for determining whether a decision can be taken without violating any restrictions and to quantify the value of taking the decision.

### Local evaluator
The @ref BPMNOS::Execution::LocalEvaluator "Local evaluator" makes local observations to evaluate a decision.

### Guided evaluator
The @ref BPMNOS::Execution::GuidedEvaluator "Guided evaluator" makes local observations and includes @ref BPMNOS::Model::Guidance "guidance" provided with the model to evaluate a decision.
