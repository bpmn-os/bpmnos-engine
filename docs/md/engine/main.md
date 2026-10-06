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

The events the environment of a run gives rise to reach the engine through the @ref BPMNOS::Execution::Environment "environment", which forwards every notification of the engine and every call for an event to the @ref BPMNOS::Execution::DataProvider "data provider" of the scenario of the run, and dispatches the events the data provider enqueues in the order in which they are enqueued. The engine runs a @ref BPMNOS::Execution::Scenario "scenario" created by a data provider, of which it takes ownership when passed to @ref BPMNOS::Execution::Engine::run "run", @ref BPMNOS::Execution::Engine::initialize "initialize" or @ref BPMNOS::Execution::Engine::initializeSystemState "initializeSystemState". A @ref BPMNOS::Model::Scenario "scenario" of the existing data providers is wrapped in a scenario of the @ref BPMNOS::Execution::LegacyDataProvider "legacy data provider", which determines these events from it. The engine obtains the global values and the earliest instantiation time of a run from the data provider of its scenario as well, and refuses a scenario whose data provider is built on another model.

Every round of the engine first asks the environment to dispatch an event due at the current time, then the dispatchers of the controller in the order in which they are connected, and, if neither supplies one, asks the environment to advance, upon which the data provider decides whether time advances: it enqueues the next @ref BPMNOS::Execution::ClockTickEvent "clock tick" at once, or, if it is given a number of milliseconds between two clock ticks, once the wall clock has reached the next one. If a round yields no event, the engine pauses for @ref BPMNOS::Execution::Engine::SLEEP "a moment" before the next. A run ends only when a @ref BPMNOS::Execution::TerminationEvent "termination event" is processed, which the legacy data provider enqueues once a clock tick leaves the scenario completed and no instance; until the end time is set on the data provider, the engine also stops before a clock tick beyond the end time it is given.

The @ref BPMNOS::Execution::StaticDataProvider "static data provider" reads instance data known from the start of a run from a CSV table with the columns `INSTANCE_ID`, `NODE_ID` and `INITIALIZATION`. Every instance is instantiated at the first instant of a run and its process becomes ready at its instantiation time; a token arriving at an activity becomes ready at once, and a task completes with the status it became busy with at the timestamp of that status. Events due later are scheduled and enqueued when the @ref BPMNOS::Execution::ClockTickEvent "clock tick event" advancing to their time is announced, and an event that has expired before it is dispatched is discarded. A run ends when nothing is left to do and no instance is left, or when nothing is left to do at the end time set by @ref BPMNOS::Execution::StaticDataProvider::setEndTime "setEndTime"; a run ended at the end time continues when the end time is raised and the run is resumed.

```cpp
auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
auto dataProvider = std::make_shared<BPMNOS::Execution::StaticDataProvider>(model, "scenario.csv");
BPMNOS::Execution::Engine engine(model);
// connect the controller as above, no clock dispatcher being needed
engine.run(dataProvider->createScenario());
```

The engine executes a single model for its whole lifetime and shares its ownership with the data provider, which returns it by @ref BPMNOS::Model::DataProvider::getModel "getModel". An engine constructed without a model takes the model of the first scenario it is given, without sharing its ownership, so that the model must then outlive the engine. In either case the engine refuses with `std::invalid_argument` a scenario created by a data provider built on another model, even if that model was parsed from the same file. Tokens, state machines and the system state reach the model through the engine.

A @ref BPMNOS::Execution::Signal "signal" that does not arise in the model is raised by the environment: the @ref BPMNOS::Execution::Environment "environment" dispatches a @ref BPMNOS::Execution::SignalBroadcastEvent "signal broadcast event" for every signal the scenario reports, and processing the event delivers the signal like one thrown within the model, without advancing time.
