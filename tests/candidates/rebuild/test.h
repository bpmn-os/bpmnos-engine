#include "../util.h"

// Tests of the candidate collections' response to a `notice(SystemState)` — the rebuild path used when a foreign
// system state is installed via Engine::initializeSystemState. A normal run only ever feeds a collection
// incremental notices and reads the front of it, so the rebuild path is otherwise untested. A collection that
// fails to rebuild offers no decision, and the engine then clock-ticks forever (a clocktick is dispatched only
// when no decision is made).
//
// For each candidate type the suite asserts the same invariants:
//   1. rebuild == incremental — the candidate set rebuilt from a single SystemState notice equals the set built
//      incrementally while the state was produced (the property a resumed run relies on);
//   2. a concrete count anchor, so the invariant cannot pass vacuously on two empty sets;
//   3. reward-descending order — best first, infeasible (-inf) last.

SCENARIO( "MessageDeliveries rebuilds its candidates from a SystemState notice", "[candidates][rebuild][message]" ) {
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A job-shop state at t=0 with three job requests created and three catch events pending" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3,2,2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2,1,4]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4,3]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    // Drive the state to t=0 with instant entry/exit and no message handler, so the requests are created and
    // their deliveries stay pending. A MessageDeliveries collection observes incrementally during this run.
    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    Execution::LocalEvaluator evaluator;
    Execution::MessageDeliveries incremental(&evaluator);
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->messages.size() == 3 );
    REQUIRE( systemState->pendingMessageDeliveryDecisions.count() == 3 );

    WHEN( "A fresh MessageDeliveries collection is rebuilt from the SystemState notice via a plain Notifier" ) {
      Execution::Notifier notifier;
      Execution::MessageDeliveries rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "It offers exactly the feasible request/message deliveries (Machine1 x2, Machine2 x1, Machine3 x0)" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) == 3 );
        REQUIRE( rebuiltCandidates.size() == 3 );
      }
      AND_THEN( "The rebuilt candidates equal the incrementally built ones, best first" ) {
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
        REQUIRE( rewardDescending(rebuiltCandidates) );
      }
    }
  }
}

SCENARIO( "CompetingCandidates rebuilds its merged candidates from a SystemState notice", "[candidates][rebuild][competing]" ) {
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A job-shop state at t=0 with three pending message deliveries and no pending sequential entries" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3,2,2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2,1,4]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4,3]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    Execution::LocalEvaluator evaluator;
    Execution::CompetingCandidates incremental(&evaluator);
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();

    WHEN( "A fresh CompetingCandidates collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::CompetingCandidates rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collectCompeting(rebuilt);
      auto incrementalCandidates = collectCompeting(incremental);

      THEN( "The merge offers the three message deliveries (sequential-entry side empty here)" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) == 3 );
        REQUIRE( rebuiltCandidates.size() == 3 );
      }
      AND_THEN( "The rebuilt merge equals the incrementally built merge and is reward-ordered" ) {
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
        REQUIRE( rewardDescending(rebuiltCandidates) );
      }
    }
  }
}

SCENARIO( "SequentialEntries rebuilds its candidates from a SystemState notice", "[candidates][rebuild][sequential]" ) {
  const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/travelling_salesperson_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile, folders) );

  GIVEN( "A TSP state at t=0 whose sequential ad-hoc children await entry" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; TravellingSalesperson_Process; speed := 1\n"
      "Instance1; TravellingSalesperson_Process; origin := \"Hamburg\"\n"
      "Instance1; TravellingSalesperson_Process; locations := [\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, folders, csv);
    auto scenario = dataProvider.createScenario();

    // FirstFeasibleEntry (config.sequential=false) enters the ad-hoc subprocess but skips its sequential
    // children, leaving their entries pending — InstantEntry would have entered them, so it cannot be used here.
    Execution::Engine engine;
    Execution::LocalEvaluator evaluator;
    Execution::GreedyDispatcher<Execution::FirstFeasibleEntry> entryHandler(&evaluator);
    Execution::TimeWarp timeHandler;
    Execution::SequentialEntries incremental(&evaluator);
    entryHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingEntryDecisions.count() >= 1 );

    WHEN( "A fresh SequentialEntries collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::SequentialEntries rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "Every pending sequential entry is offered as a feasible candidate" ) {
        REQUIRE( rebuiltCandidates.size() == systemState->pendingEntryDecisions.count() );
        REQUIRE( feasibleCount(rebuiltCandidates) >= 1 );
      }
      AND_THEN( "The rebuilt candidates equal the incrementally built ones and are reward-ordered" ) {
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
        REQUIRE( rewardDescending(rebuiltCandidates) );
      }
    }
  }
}

SCENARIO( "FirstFeasibleEntry rebuilds its candidate from a SystemState notice", "[candidates][rebuild][entry]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A state at t=0 with an activity awaiting an entry decision (no entry handler)" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::TimeWarp timeHandler;
    Execution::LocalEvaluator evaluator;
    Execution::FirstFeasibleEntry incremental(&evaluator);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingEntryDecisions.count() == 1 );

    WHEN( "A fresh FirstFeasibleEntry collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::FirstFeasibleEntry rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "It offers the single feasible entry, matching the incremental build" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) == 1 );
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
      }
    }
  }
}

SCENARIO( "FirstFeasibleExit rebuilds its candidate from a SystemState notice", "[candidates][rebuild][exit]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A state at t=0 with a completed activity awaiting an exit decision (no exit handler)" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    // Enter and make the choice instantly, but withhold the exit handler so the exit decision stays pending.
    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::LocalEvaluator evaluator;
    Execution::GreedyDispatcher<Execution::FirstEnumeratedChoice> choiceHandler(&evaluator);
    Execution::TimeWarp timeHandler;
    Execution::FirstFeasibleExit incremental(&evaluator);
    entryHandler.connect(&engine);
    choiceHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingExitDecisions.count() == 1 );

    WHEN( "A fresh FirstFeasibleExit collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::FirstFeasibleExit rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "It offers the single feasible exit, matching the incremental build" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) == 1 );
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
      }
    }
  }
}

SCENARIO( "FirstEnumeratedChoice rebuilds its candidates from a SystemState notice", "[candidates][rebuild][choice]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A state at t=0 with a pending choice request (no choice handler)" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    Execution::LocalEvaluator evaluator;
    Execution::FirstEnumeratedChoice incremental(&evaluator);
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingChoiceDecisions.count() == 1 );

    WHEN( "A fresh FirstEnumeratedChoice collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::FirstEnumeratedChoice rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "It enumerates feasible choice alternatives, matching the incremental build, best first" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) >= 1 );
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
        REQUIRE( rewardDescending(rebuiltCandidates) );
      }
    }
  }
}

SCENARIO( "FirstBisectionalChoice rebuilds its candidates from a SystemState notice", "[candidates][rebuild][choice][bisection]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_bounds.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A state at t=0 with a pending bounded choice request (no choice handler)" ) {
    // The bounds model defines the choice's bounds internally, so the instance needs no input values.
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    Execution::LocalEvaluator evaluator;
    Execution::FirstBisectionalChoice incremental(&evaluator);
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    incremental.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingChoiceDecisions.count() == 1 );

    WHEN( "A fresh FirstBisectionalChoice collection is rebuilt from the SystemState notice" ) {
      Execution::Notifier notifier;
      Execution::FirstBisectionalChoice rebuilt(&evaluator);
      rebuilt.connect(&notifier);
      notifier.notify(systemState);

      auto rebuiltCandidates = collect(rebuilt);
      auto incrementalCandidates = collect(incremental);

      THEN( "Its bisection search yields a feasible choice, matching the incremental build, best first" ) {
        REQUIRE( feasibleCount(rebuiltCandidates) >= 1 );
        REQUIRE( signatures(rebuiltCandidates) == signatures(incrementalCandidates) );
        REQUIRE( rewardDescending(rebuiltCandidates) );
      }
    }
  }
}
