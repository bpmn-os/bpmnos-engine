#include <chrono>

/**
 * Test data provider enqueuing, in addition to the events of the legacy data provider, a termination event
 * once the given time is reached.
 */
class TestDataProvider : public Execution::LegacyDataProvider {
public:
  TestDataProvider(const Model::Model* model, unsigned int clockTickDuration, std::optional<BPMNOS::number> terminationTime = std::nullopt)
    : LegacyDataProvider(model, clockTickDuration)
    , terminationTime(terminationTime)
  {
  }

  void dispatchEvent(const Execution::SystemState* systemState, Execution::Scenario& scenario, Execution::EventQueue& queue) const override {
    if ( terminationTime.has_value() && systemState->getTime() >= terminationTime.value() ) {
      queue.push_back(std::make_shared<Execution::TerminationEvent>());
      return;
    }
    LegacyDataProvider::dispatchEvent(systemState, scenario, queue);
  }

  const std::optional<BPMNOS::number> terminationTime;
};

SCENARIO( "Clock ticks and termination supplied by the data provider", "[data][provider]" ) {
  const std::string modelFile = "tests/execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance whose timer is triggered at time 10" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;
    Model::StaticDataProvider modelDataProvider(modelFile,csv);
    auto modelScenario = modelDataProvider.createScenario();

    // the reference run, in which the clock ticks are supplied by a time warp
    Execution::Engine referenceEngine;
    Execution::InstantEntry referenceEntryHandler;
    Execution::InstantExit referenceExitHandler;
    Execution::TimeWarp timeHandler;
    referenceEntryHandler.connect(&referenceEngine);
    referenceExitHandler.connect(&referenceEngine);
    timeHandler.connect(&referenceEngine);
    Execution::Recorder referenceRecorder;
    referenceRecorder.subscribe(&referenceEngine);
    referenceEngine.run(modelScenario.get());

    WHEN( "The engine runs without a clock dispatcher on a data provider without clock tick duration" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario(modelScenario.get()));

      THEN( "Every decision of the controller precedes the clock tick following it, as with a time warp" ) {
        REQUIRE( recorder.log == referenceRecorder.log );
      }
      THEN( "The run ends exactly when the termination event is processed" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.size() == 1 );
      }
    }

    WHEN( "The engine runs without a clock dispatcher on a data provider with a clock tick duration" ) {
      const unsigned int clockTickDuration = 20;
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), clockTickDuration);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      auto scenario = dataProvider->createScenario(modelScenario.get());
      auto start = std::chrono::steady_clock::now();
      engine.run(std::move(scenario));
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

      THEN( "Every decision of the controller is processed before the next clock tick, as with a time warp" ) {
        REQUIRE( recorder.log == referenceRecorder.log );
      }
      THEN( "The clock ticks follow the wall clock" ) {
        // the opening clock tick is processed by the engine, every later one is due the given duration
        // after the previous one
        auto clockTicks = recorder.find(nlohmann::json{{"event","clocktick"}}).size();
        REQUIRE( clockTicks > 2 );
        REQUIRE( (size_t)elapsed >= ( clockTicks - 2 ) * clockTickDuration );
      }
    }

    WHEN( "The engine runs on a data provider terminating the run at time 5" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0, 5);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario(modelScenario.get()));

      THEN( "The run ends when the termination event is processed, with the instance still running" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 5 );
        REQUIRE( engine.getSystemState()->instances.size() == 1 );
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.empty() );
      }
    }
  }
}

SCENARIO( "No clock tick before the first request to the controller after installing a system state", "[data][provider]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A system state in which a token awaits the decision to enter a task" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    Model::StaticDataProvider modelDataProvider(modelFile,csv);
    auto modelScenario = modelDataProvider.createScenario();

    // without an entry handler the token awaits the entry decision, and the run stops before time 1
    Execution::Engine sourceEngine;
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&sourceEngine);
    sourceEngine.run(modelScenario.get(), 0, 0);
    REQUIRE_FALSE( sourceEngine.getSystemState()->pendingEntryDecisions.empty() );

    WHEN( "The system state is installed in an engine without a clock dispatcher and the engine advances" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.initializeSystemState(dataProvider->createScenario(modelScenario.get()), sourceEngine.getSystemState());
      while ( recorder.find(nlohmann::json{{"event","entry"}}).empty() && recorder.find(nlohmann::json{{"event","clocktick"}}).empty() ) {
        REQUIRE( engine.advance() );
      }

      THEN( "The entry event is processed before any clock tick" ) {
        REQUIRE_FALSE( recorder.find(nlohmann::json{{"event","entry"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"event","clocktick"}}).empty() );
      }
    }
  }
}
