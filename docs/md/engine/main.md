# Execution engine
@page engine Execution engine


@todo Describe required @ref BPMNOS::Execution::EventDispatcher.


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


