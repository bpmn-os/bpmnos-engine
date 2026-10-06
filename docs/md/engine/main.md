# Execution engine
@page engine Execution engine

The execution engine is responsible for running a BPMN model instance. It requires a data provider, several @ref BPMNOS::Execution::EventDispatcher "event dispatchers" to provide execution relevant information during runtime, and a @ref BPMNOS::Execution::Controller "controller" making decisions during execution.

The engine internally handles:
- Instantiation events, ready events, task completion events and scenario data updates via @ref BPMNOS::Execution::Environment "Environment"

Below is an example using
a @ref BPMNOS::Execution::StaticDataProvider "static data provider" to obtain a scenario, which also advances the simulation time,
and a @ref BPMNOS::Execution::GreedyController "greedy controller" using a @ref BPMNOS::Execution::GuidedEvaluator "guided evaluator".

```cpp
#include <bpmnos-model.h>
#include <bpmnos-execution.h>

int main() {
  // load model and instances
  auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::StaticDataProvider>(model, "scenario.csv");

  // initialize execution engine with the model
  BPMNOS::Execution::Engine engine(model);

  // initialize and connect BPMNOS::Execution::Controller for BPMNOS::Execution::Decision
  auto evaluator = std::make_shared<BPMNOS::Execution::GuidedEvaluator>();
  BPMNOS::Execution::GreedyController controller(evaluator);
  controller.connect(&engine);

  // run engine on a scenario of the data provider
  engine.run(dataProvider->createScenario());
}
```

The events the environment of a run gives rise to reach the engine through the @ref BPMNOS::Execution::Environment "environment", which forwards every notification of the engine and every call for an event to the @ref BPMNOS::Execution::DataProvider "data provider" of the scenario of the run, and dispatches the events the data provider enqueues in the order in which they are enqueued. The engine runs a @ref BPMNOS::Execution::Scenario "scenario" created by a data provider, of which it takes ownership when passed to @ref BPMNOS::Execution::Engine::run "run", @ref BPMNOS::Execution::Engine::initialize "initialize" or @ref BPMNOS::Execution::Engine::initializeSystemState "initializeSystemState". A @ref BPMNOS::Model::Scenario "scenario" of the existing data providers is wrapped in a scenario of the @ref BPMNOS::Execution::LegacyDataProvider "legacy data provider", which determines these events from it. The engine obtains the global values and the earliest instantiation time of a run from the data provider of its scenario as well, and refuses a scenario whose data provider is built on another model.

Every round of the engine first asks the environment to dispatch an event due at the current time, then the dispatchers of the controller in the order in which they are connected, and, if neither supplies one, asks the environment to advance, upon which the data provider decides whether time advances: it enqueues the next @ref BPMNOS::Execution::ClockTickEvent "clock tick" at once, or, if it is given a number of milliseconds between two clock ticks, once the wall clock has reached the next one. If a round yields no event, the engine pauses for @ref BPMNOS::Execution::Engine::SLEEP "a moment" before the next. A run ends only when a @ref BPMNOS::Execution::TerminationEvent "termination event" is processed, and which a data provider enqueues once nothing further is to come, or once nothing is left to do at its end time. The engine itself knows no end time; a run on a @ref BPMNOS::Model::Scenario "scenario" of the existing data providers is given its end time through the legacy data provider that wraps it.

The @ref BPMNOS::Execution::StaticDataProvider "static data provider" reads instance data known from the start of a run from a CSV table with the columns `INSTANCE_ID`, `NODE_ID` and `INITIALIZATION`. Every instance is instantiated at the first instant of a run and its process becomes ready at its instantiation time; a token arriving at an activity becomes ready at once, and a task completes with the status it became busy with at the timestamp of that status. Events due later are scheduled and enqueued when the @ref BPMNOS::Execution::ClockTickEvent "clock tick event" advancing to their time is announced, and an event that has expired before it is dispatched is discarded. A run ends when nothing is left to do and no instance is left, or when nothing is left to do at the end time set by @ref BPMNOS::Execution::StaticDataProvider::setEndTime "setEndTime"; a run ended at the end time continues when the end time is raised and the run is resumed.

The @ref BPMNOS::Execution::ExpectedValueDataProvider "expected value data provider" proceeds as the static data provider, but reads a table that may also have the columns `DISCLOSURE`, `READY` and `COMPLETION`, which it ignores, and evaluates every initialization with the expected values of its random functions.

The @ref BPMNOS::Execution::DynamicDataProvider "dynamic data provider" proceeds as the static data provider for data disclosed during a run. Its table may have the column `DISCLOSURE`, an expression giving the time, rounded up, at which the value of its row is disclosed. The disclosure time of a node is the latest disclosure of its rows and of the scope containing it. An instance becomes known at the disclosure time of its process, its process becomes ready at its instantiation time but not before, and a token arriving at an activity becomes ready at the disclosure time of the activity but not before.

The @ref BPMNOS::Execution::StochasticDataProvider "stochastic data provider" samples the instance data from expressions with random functions. Its table may also have the columns `READY` and `COMPLETION`, expressions assigning a status attribute of an activity and of a task. The global values are sampled once, with the seed of the data provider, and the initializations and disclosures of the instances are sampled for every scenario, with the seed of the data provider plus the realisation the scenario is created for. A run proceeds as on the dynamic data provider, except that a token arriving at an activity becomes ready with the status the ready expressions give, once its timestamp is reached, and a task completes with the status the completion expressions give. A run on a scenario of the stochastic data provider is forked by @ref BPMNOS::Execution::DataProvider::forkScenario "forkScenario", which creates the scenario of a fork with a given index from the scenario of the run: it keeps every initialization disclosed before the instant following the current time of the run and samples every other one anew with the seed of the run plus the index plus one, and the statuses the tokens of the installed state become ready and complete with are sampled anew when the system state of the run is installed together with the fork. The other data providers refuse to fork, their future being certain; a run on them is continued on a new scenario of its data provider.

```cpp
auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
auto dataProvider = std::make_shared<BPMNOS::Execution::StaticDataProvider>(model, "scenario.csv");
BPMNOS::Execution::Engine engine(model);
// connect the controller as above, no clock dispatcher being needed
engine.run(dataProvider->createScenario());
```

The engine executes a single model for its whole lifetime and shares its ownership with the data provider, which returns it by @ref BPMNOS::Model::DataProvider::getModel "getModel". An engine constructed without a model takes the model of the first scenario it is given, without sharing its ownership, so that the model must then outlive the engine. In either case the engine refuses with `std::invalid_argument` a scenario created by a data provider built on another model, even if that model was parsed from the same file. Tokens, state machines and the system state reach the model through the engine.

A @ref BPMNOS::Execution::Signal "signal" that does not arise in the model is raised by the environment: the @ref BPMNOS::Execution::Environment "environment" dispatches a @ref BPMNOS::Execution::SignalBroadcastEvent "signal broadcast event" for every signal the scenario reports, and processing the event delivers the signal like one thrown within the model, without advancing time.
