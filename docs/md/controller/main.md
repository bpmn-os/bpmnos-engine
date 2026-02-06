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

The @ref BPMNOS::Execution::GreedyController::connect "greedy controller" implements a @ref BPMNOS::Execution::Controller "controller" with a collection of @ref BPMNOS::Execution::GreedyDispatcher "greedy dispatchers":
- the @ref BPMNOS::Execution::BestFirstEntry "best first entry" dispatcher evaluates entry decisions for all activities and dispatches the entry decision with the best evaluation,
- @ref BPMNOS::Execution::BestFirstExit "best first exit" dispatcher evaluates exit decisions for all activities and dispatches the exit decision with the best evaluation,
- @ref BPMNOS::Execution::BestMatchingMessageDelivery "best matching message delivery" dispatcher evaluates message delivery decisions for all @ref BPMNOS::Execution::Message "sent messages" and @ref BPMN::MessageCatchEvent "potential recipients" and dispatches the message delivery decision with the best evaluation.
- the @ref BPMNOS::Execution::BestEnumeratedChoice "best enumerated choice dispatcher" evaluates all choices that can be enumerated. In case, of numeric choices lower bound and upper bound as well a discretize condition must be provided, e.g. `lb <= choice <= ub, 5 divides choice` where `lb` and `ub` may be arbitrary expressions and `choice` is the attribute name for which the choice is to be made. The discretize condition accepts a numeric value or an expression. If no discretize condition is provided only minimum and maximum values are considered.

Among all decisions dispatched by any of the greedy dispatchers, the greedy controller selects the decision with the overall best evaluation.

All evaluations are conducted by an @ref BPMNOS::Execution::Evaluator "evaluator".

## Evaluator

The @ref BPMNOS::Execution::Evaluator "evaluator" is responsible for determining whether a decision can be taken without violating any restrictions and to quantify the value of taking the decision.

### Local evaluator
The @ref BPMNOS::Execution::LocalEvaluator "Local evaluator" makes local observations to evaluate a decision.

### Guided evaluator
The @ref BPMNOS::Execution::GuidedEvaluator "Guided evaluator" makes local observations and includes @ref BPMNOS::Model::Guidance "guidance" provided with the model to evaluate a decision.
