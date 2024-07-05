# Execution engine
@page engine Execution engine

The execution engine is responsible for running a BPMN model instance. It requires a data provider, several @ref BPMNOS::Execution::EventDispatcher "event dispatchers" to provide execution relevant information during runtime, and a @ref BPMNOS::Execution::Controller "controller" making decisions during execution.
Below is an example using
a @ref BPMNOS::Model::StaticDataProvider "static data provider" to obtain a scenario,
a @ref BPMNOS::Execution::ReadyHandler "ready handler", to provide activity-related information,
a @ref BPMNOS::Execution::DeterministicTaskCompletion "deterministic completion handler", to trigger task completion, and
a @ref BPMNOS::Execution::TimeWarp "time warp handler", to trigger the advancement of the simulation time,
and a @ref BPMNOS::Execution::GreedyController "greedy controller" using a @ref BPMNOS::Execution::GuidedEvaluator "guided evaluator".

```cpp
#include <bpmnos-model.h>
#include <bpmnos-execution.h>

int main() {
  // load model and instances
  BPMNOS::Model::StaticDataProvider dataProvider("diagram.bpmn","scenario.csv");
  auto scenario = dataProvider.createScenario();

  // initialize execution engine
  BPMNOS::Execution::Engine engine;

  // initialize and connect BPMNOS::Execution::EventDispatcher for BPMNOS::Execution::ReadyEvent
  BPMNOS::Execution::ReadyHandler readyHandler;
  readyHandler.connect(&engine);

  // initialize and connect BPMNOS::Execution::EventDispatcher for BPMNOS::Execution::CompletionEvent
  BPMNOS::Execution::DeterministicTaskCompletion completionHandler;
  completionHandler.connect(&engine);

  // initialize and connect BPMNOS::Execution::EventDispatcher for BPMNOS::Execution::ClockTickEvent
  BPMNOS::Execution::TimeWarp timeHandler;
  timeHandler.connect(&engine);

  // initialize and connect BPMNOS::Execution::Controller for BPMNOS::Execution::Decision
  BPMNOS::Execution::GuidedEvaluator evaluator;
  BPMNOS::Execution::GreedyController controller(&evaluator);
  controller.connect(&engine);

  // run engine on scenario
  engine.run(scenario.get());
}
```
