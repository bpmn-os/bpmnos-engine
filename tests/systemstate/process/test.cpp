#include "prelude.h"

SCENARIO( "SystemState copy for simple process", "[systemstate][process]" ) {
  const std::string modelFile = "tests/systemstate/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An engine with two running instances" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    // the engine enters the tasks itself in the round after they become ready, so that the run is advanced round
    // by round until both await entry
    auto awaitingEntry = [](const Execution::SystemState* systemState) {
      size_t count = 0;
      for ( auto& [instanceId, waitingTokens] : systemState->tokensAwaitingEntry ) {
        count += waitingTokens.count();
      }
      return count;
    };
    Execution::Engine engine(model);
    Execution::Recorder recorder;
//   Execution::Recorder recorder(std::cerr);  // prints for debugging
    recorder.subscribe(&engine);

    engine.initialize(std::move(scenario), 0);
    while ( awaitingEntry(engine.getSystemState()) < 2 ) {
      REQUIRE( engine.advance() );
    }
    const auto* originalState= engine.getSystemState();

    REQUIRE( originalState->stateMachine->tokens.size() == 2 );
    REQUIRE( awaitingEntry(originalState) == 2 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      //
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same number of instances" ) {
        REQUIRE( copiedState.stateMachine->tokens.size() == originalState->stateMachine->tokens.size() );
      }
      THEN( "The archive has the same number of active entries" ) {
        // Lambda to count active (non-expired) archive entries
        auto count = [](const auto& archive) {
          return std::ranges::count_if(
            archive | std::views::values,
            [](const auto& wp) { return !wp.expired(); });
        };


        REQUIRE( count(copiedState.archive) == count(originalState->archive) );
      }


      THEN( "The copy has the same numeric values" ) {
        REQUIRE( copiedState.currentTime == originalState->currentTime );
        REQUIRE( copiedState.stateMachine->ownedData.attributes.size() == originalState->stateMachine->ownedData.attributes.size() );
      }

      THEN( "The copy has the same tokens awaiting entry" ) {
        REQUIRE( awaitingEntry(&copiedState) == awaitingEntry(originalState) );
      }
    }
  }
}

SCENARIO( "SystemState copy with token awaiting ready event", "[systemstate][process][ready]" ) {
  const std::string modelFile = "tests/systemstate/process/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with activity data disclosed in the future" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_1; Activity_1; y := 0; 10\n"
      "Instance_1; Activity_1; data := 0; 10\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    entryHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    dataProvider->setEndTime(0);
    engine.run(std::move(scenario), 0);
    const auto* originalState = engine.getSystemState();

    // Token should be waiting for ready event at Activity_1
    REQUIRE( originalState->tokensAwaitingReadyEvent.count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting ready event" ) {
        REQUIRE( copiedState.tokensAwaitingReadyEvent.count() ==
                 originalState->tokensAwaitingReadyEvent.count() );
      }
    }
  }
}

SCENARIO( "Engine resume from stopped state", "[systemstate][process][resume]" ) {
  const std::string modelFile = "tests/systemstate/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance that stops with its task awaiting entry" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);

    // First engine: advanced round by round until the task awaits entry, which the engine takes in the next round
    Execution::Engine engine1(model);
    Execution::Recorder recorder1;
    recorder1.subscribe(&engine1);

    engine1.initialize(dataProvider->createScenario(), 0);
    while ( engine1.getSystemState()->tokensAwaitingEntry.empty() ) {
      REQUIRE( engine1.advance() );
    }

    REQUIRE( engine1.getSystemState()->getTime() == 0 );

    // Token should be at READY state, awaiting entry
    auto activityLog1 = recorder1.find(nlohmann::json{{"nodeId","Activity_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
    REQUIRE( activityLog1.back()["state"] == "READY" );

    WHEN( "A second engine resumes with a fork of the scenario" ) {
      // Fork the scenario of the first engine's run, which differs from it at the next point in time and
      // takes the first realization other than the one that run itself produced
      auto fork = dataProvider->forkScenario(*engine1.getSystemState()->scenario, 0);

      // Second engine, which enters the task itself
      Execution::Engine engine2(model);
      Execution::Recorder recorder2;
      recorder2.subscribe(&engine2);

      // Resume from a copy of the first engine's state with the fork
      engine2.initializeSystemState(std::move(fork), engine1.getSystemState());
      engine2.resume();

      THEN( "The process completes successfully" ) {
        auto processLog = recorder2.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr}, {"event",nullptr}, {"decision",nullptr}});
        REQUIRE( processLog.back()["state"] == "DONE" );

        auto activityLog2 = recorder2.find(nlohmann::json{{"nodeId","Activity_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
        REQUIRE( activityLog2.back()["state"] == "DEPARTED" );

        auto endLog = recorder2.find(nlohmann::json{{"nodeId","EndEvent_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
        REQUIRE( endLog.back()["state"] == "DONE" );
      }
    }
  }
}
