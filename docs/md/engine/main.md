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

The events the environment of a run gives rise to reach the engine through the @ref BPMNOS::Execution::Environment "environment", which forwards every notification of the engine and every call for an event to the @ref BPMNOS::Execution::DataProvider "data provider" of the scenario of the run, and dispatches the events the data provider enqueues in the order in which they are enqueued. The engine wraps the @ref BPMNOS::Model::Scenario "scenario" it is given in a scenario of the @ref BPMNOS::Execution::LegacyDataProvider "legacy data provider", which determines these events from it.

Every round of the engine first asks the environment to dispatch an event due at the current time, then the dispatchers of the controller in the order in which they are connected, and, if neither supplies one, asks the environment to advance, upon which the data provider decides whether time advances: it enqueues the next @ref BPMNOS::Execution::ClockTickEvent "clock tick" at once, or, if it is given a number of milliseconds between two clock ticks, once the wall clock has reached the next one. If a round yields no event, the engine pauses for @ref BPMNOS::Execution::Engine::SLEEP "a moment" before the next. A run ends only when a @ref BPMNOS::Execution::TerminationEvent "termination event" is processed, which the legacy data provider enqueues once a clock tick leaves the scenario completed and no instance; until the end time is set on the data provider, the engine also stops before a clock tick beyond the end time it is given.

The engine executes a single model for its whole lifetime and shares its ownership with the data provider, which returns it by @ref BPMNOS::Model::DataProvider::getModel "getModel". An engine constructed without a model takes the model of the first scenario it is given, without sharing its ownership, so that the model must then outlive the engine. In either case the engine refuses with `std::invalid_argument` a scenario created by a data provider built on another model, even if that model was parsed from the same file. Tokens, state machines and the system state reach the model through the engine.

A @ref BPMNOS::Execution::Signal "signal" that does not arise in the model is raised by the environment: the @ref BPMNOS::Execution::Environment "environment" dispatches a @ref BPMNOS::Execution::SignalBroadcastEvent "signal broadcast event" for every signal the scenario reports, and processing the event delivers the signal like one thrown within the model, without advancing time.
