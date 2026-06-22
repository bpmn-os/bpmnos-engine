#include "../util.h"

// Tests of resuming a run from an installed (copied) system state via Engine::initializeSystemState + resume.
// A normal run completes; resuming an equivalent installed state must complete the same way. A finite time bound
// turns a non-terminating clock-tick livelock (the engine dispatches a clocktick only when no decision is made)
// into an assertable failure instead of a hang. The job-shop variants shrink the problem to isolate which
// structural feature (multiplicity, repeated completion keys, sequential multi-instance loops) breaks resumption.

SCENARIO( "GreedyController terminates when resumed from an installed system state", "[candidates][resume][install]" ) {
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A job-shop state at t=0 with three pending job-request deliveries" ) {
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
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingMessageDeliveryDecisions.count() == 3 );

    WHEN( "The state is installed into a fresh engine and resumed under the greedy controller" ) {
      // Resume against the original, already-revealed scenario, so taskCompletionStatus is correct and any
      // stall is in the engine's resume logic rather than scenario disclosure.
      Execution::Engine resumed;
      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&resumed);
      Execution::TimeWarp resumedTimeHandler;
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);

      // Localize the defect: does the *copied* state still carry the deliveries the candidate rebuild needs?
      const auto* copiedState = resumed.getSystemState();
      THEN( "The copied state preserves the three messages and three pending deliveries" ) {
        REQUIRE( copiedState->messages.size() == 3 );
        REQUIRE( copiedState->pendingMessageDeliveryDecisions.count() == 3 );
      }
      AND_THEN( "MessageDeliveries rebuilt from the copied state still offers three deliveries" ) {
        Execution::Notifier notifier;
        Execution::MessageDeliveries copyCandidates(&evaluator);
        copyCandidates.connect(&notifier);
        notifier.notify(copiedState);
        REQUIRE( feasibleCount(collect(copyCandidates)) == 3 );
      }

      resumed.resume(100);   // finite bound: a livelock stops here instead of hanging

      THEN( "The run reaches a real end state well before the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
      }
      AND_THEN( "All six process instances complete" ) {
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.size() == 3 + 3 );
      }
    }
  }
}

SCENARIO( "GreedyController terminates when resumed from a minimal one-job state", "[candidates][resume][minimal]" ) {
  // Minimal shrink of the job shop: one machine, one order, one job — same event-subprocess + message-completion
  // structure, but no multiplicity or repeated (instance,node) completion keys. Isolates whether the resume stall
  // needs multiple jobs or appears with a single one.
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A one-machine one-order one-job state at t=0 with a single pending request delivery" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 1\n"
      "Order1; OrderProcess; machines := [\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->pendingMessageDeliveryDecisions.count() == 1 );

    WHEN( "The state is installed into a fresh engine and resumed under the greedy controller" ) {
      Execution::Engine resumed;
      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&resumed);
      Execution::TimeWarp resumedTimeHandler;
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);
      resumed.resume(100);

      THEN( "The run reaches a real end state well before the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
      }
      AND_THEN( "Both process instances complete" ) {
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.size() == 1 + 1 );
      }
    }
  }
}

SCENARIO( "GreedyController terminates when resumed from a one-machine two-job state", "[candidates][resume][twojobs]" ) {
  // One machine conducting two jobs (jobs:=2) from two orders — the same (instance, ConductJobTask) completion
  // key is reused for both conducts. Isolates whether the repeated completion key breaks the resumed run.
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One machine (jobs:=2) and two orders, each with a single job for that machine" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Order1; OrderProcess; machines := [\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\"]\n"
      "Order2; OrderProcess; durations := [3]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->messages.size() == 2 );   // both orders sent their request to Machine1

    WHEN( "The state is installed into a fresh engine and resumed under the greedy controller" ) {
      Execution::Engine resumed;
      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&resumed);
      Execution::TimeWarp resumedTimeHandler;
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);
      resumed.resume(100);

      THEN( "The run reaches a real end state well before the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
      }
      AND_THEN( "All three process instances complete" ) {
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.size() == 1 + 2 );
      }
    }
  }
}

SCENARIO( "GreedyController terminates when resumed from a one-order two-sequential-job state", "[candidates][resume][orderloop]" ) {
  // One order performing two sequential jobs (its JobActivity multi-instance loop), both on one machine.
  // Isolates whether resuming an order mid-way through its sequential job loop continues past the first job.
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One machine (jobs:=2) and one order with two sequential jobs for that machine" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2,3]\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    REQUIRE( systemState->messages.size() == 1 );   // only the first job's request is sent at t=0

    WHEN( "The same scenario is run from the start under the greedy controller (baseline, no install/resume)" ) {
      // Validates that the expected outcome below is correct independent of the install/resume path: a plain
      // greedy run of this model completes both instances. If this passes but the resumed run does not, the
      // resume path is at fault rather than the test's expectation.
      auto baselineScenario = dataProvider.createScenario();
      Execution::Engine baseline;
      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&baseline);
      Execution::TimeWarp baselineTimeHandler;
      baselineTimeHandler.connect(&baseline);
      Execution::Recorder recorder;
      recorder.subscribe(&baseline);

      baseline.run(baselineScenario.get(), 100);   // finite bound: a non-terminating run stops here

      THEN( "Both process instances complete from the start" ) {
        REQUIRE( (double)baseline.getSystemState()->getTime() < 100.0 );
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.size() == 1 + 1 );
      }
    }

    WHEN( "The state is installed into a fresh engine and resumed under the greedy controller" ) {
      Execution::Engine resumed;
      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&resumed);
      Execution::TimeWarp resumedTimeHandler;
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);
      resumed.resume(100);

      THEN( "The run reaches a real end state well before the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
      }
      AND_THEN( "Both process instances complete" ) {
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.size() == 1 + 1 );
      }
    }
  }
}

SCENARIO( "TaskCompletionHandler completes a task that was BUSY when the state was installed", "[candidates][resume][taskcompletion]" ) {
  // Minimal, message-free reproduction of the resume stall. A plain task goes BUSY at t=0 with its completion
  // scheduled at t=1. If the state is installed into a fresh engine and resumed, the task must still complete.
  // The completion time is "revealed" into the scenario's taskCompletionStatus by the ScenarioUpdater on a live
  // Token-BUSY notice; the SystemState copy constructs the BUSY token without re-emitting that notice, so if the
  // reveal is not reconstructed on install the TaskCompletionHandler waits forever and the engine clock-ticks.
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A state at t=0 with a task BUSY awaiting completion at t=1" ) {
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
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder buildRecorder;
    buildRecorder.subscribe(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    // Precondition: the task is BUSY (awaiting completion) and has not yet completed at t=0.
    REQUIRE( buildRecorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","BUSY"}}).size() == 1 );
    REQUIRE( buildRecorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}}).size() == 0 );

    WHEN( "The state is installed into a fresh engine and resumed" ) {
      // Resume against the original scenario; its taskCompletionStatus already holds Activity_1's completion,
      // so this isolates the engine's resume logic from scenario disclosure.
      Execution::Engine resumed;
      Execution::InstantEntry resumedEntry;
      Execution::InstantExit resumedExit;
      Execution::TimeWarp resumedTimeHandler;
      resumedEntry.connect(&resumed);
      resumedExit.connect(&resumed);
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);
      resumed.resume(100);   // finite bound: a stall stops here instead of hanging

      THEN( "The task completes rather than the engine clock-ticking to the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}}).size() == 1 );
      }
    }
  }
}

SCENARIO( "InstantDirectMessage delivers a directly-addressed message pending at install", "[candidates][resume][directmessage]" ) {
  // A directly-addressed message (recipient specified on the throw, sender specified on the catch) is handled by
  // InstantDirectMessage. Unlike the candidate sources, that dispatcher is stateful and — without rebuilding on a
  // SystemState notice — would not learn of a receive request that was already pending when the state was
  // installed, so the message would never be delivered and the engine would clock-tick. This installs exactly
  // such a state (receiver waiting, message created) and asserts the resumed run delivers it.
  const std::string modelFile = "tests/execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A directly-addressed message created and its receiver waiting at install" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    // No message handler: the throw creates the message and the catch event waits (delivery pending).
    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder buildRecorder;
    buildRecorder.subscribe(&engine);
    engine.run(scenario.get(), 0);

    const auto* systemState = engine.getSystemState();
    // Precondition: the message exists and the receiver is waiting (BUSY), not yet delivered.
    REQUIRE( systemState->messages.size() == 1 );
    REQUIRE( buildRecorder.find(nlohmann::json{{"nodeId","MessageCatchEvent_2"},{"state","BUSY"}}).size() == 1 );
    REQUIRE( buildRecorder.find(nlohmann::json{{"nodeId","MessageCatchEvent_2"},{"state","COMPLETED"}}).size() == 0 );

    WHEN( "The state is installed into a fresh engine with an InstantDirectMessage handler and resumed" ) {
      Execution::Engine resumed;
      Execution::InstantEntry resumedEntry;
      Execution::InstantExit resumedExit;
      Execution::InstantDirectMessage messageHandler;
      Execution::TimeWarp resumedTimeHandler;
      resumedEntry.connect(&resumed);
      resumedExit.connect(&resumed);
      messageHandler.connect(&resumed);
      resumedTimeHandler.connect(&resumed);
      Execution::Recorder recorder;
      recorder.subscribe(&resumed);

      resumed.initializeSystemState(scenario.get(), systemState);
      resumed.resume(100);   // finite bound: a non-delivery stall stops here instead of hanging

      THEN( "The pending message is delivered: the receiver completes before the time bound" ) {
        REQUIRE( (double)resumed.getSystemState()->getTime() < 100.0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","MessageCatchEvent_2"},{"state","COMPLETED"}}).size() == 1 );
      }
    }
  }
}
