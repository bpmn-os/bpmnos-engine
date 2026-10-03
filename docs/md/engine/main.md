# Execution engine
@page engine Execution engine

The execution engine is responsible for running a BPMN model instance. It requires a data provider, several @ref BPMNOS::Execution::EventDispatcher "event dispatchers" to provide execution relevant information during runtime, and a @ref BPMNOS::Execution::Controller "controller" making decisions during execution.

The engine internally handles:
- Instantiation events, ready events, task completion events and scenario data updates via @ref BPMNOS::Execution::Environment "Environment"

Below is an example using
a @ref BPMNOS::Model::StaticDataProvider "static data provider" to obtain a scenario,
a @ref BPMNOS::Execution::TimeWarp "time warp handler", to trigger the advancement of the simulation time,
and a @ref BPMNOS::Execution::GreedyController "greedy controller" using a @ref BPMNOS::Execution::GuidedEvaluator "guided evaluator".

```cpp
#include <bpmnos-model.h>
#include <bpmnos-execution.h>

int main() {
  // load model and instances
  BPMNOS::Model::StaticDataProvider dataProvider("diagram.bpmn","scenario.csv");
  auto scenario = dataProvider.createScenario();

  // initialize execution engine with the model of the data provider
  BPMNOS::Execution::Engine engine(dataProvider.getModel());

  // initialize and connect BPMNOS::Execution::EventDispatcher for BPMNOS::Execution::ClockTickEvent
  BPMNOS::Execution::TimeWarp timeHandler;
  timeHandler.connect(&engine);

  // initialize and connect BPMNOS::Execution::Controller for BPMNOS::Execution::Decision
  auto evaluator = std::make_shared<BPMNOS::Execution::GuidedEvaluator>();
  BPMNOS::Execution::GreedyController controller(evaluator);
  controller.connect(&engine);

  // run engine on scenario
  engine.run(scenario.get());
}
```

The engine executes a single model for its whole lifetime and shares its ownership with the data provider, which returns it by @ref BPMNOS::Model::DataProvider::getModel "getModel". An engine constructed without a model takes the model of the first scenario it is given, without sharing its ownership, so that the model must then outlive the engine. In either case the engine refuses with `std::invalid_argument` a scenario created by a data provider built on another model, even if that model was parsed from the same file. Tokens, state machines and the system state reach the model through the engine.

A @ref BPMNOS::Execution::Signal "signal" that does not arise in the model is raised by the environment: the @ref BPMNOS::Execution::Environment "environment" dispatches a @ref BPMNOS::Execution::SignalBroadcastEvent "signal broadcast event" for every signal the scenario reports, and processing the event delivers the signal like one thrown within the model, without advancing time. An @ref BPMNOS::Model::ObservedScenario "observed scenario" reports the signals it is told about through @ref BPMNOS::Model::ObservedScenario::observeSignal "observeSignal".
