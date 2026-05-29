SCENARIO( "SystemState copy for simple process", "[systemstate][process]" ) {
  const std::string modelFile = "tests/systemstate/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An engine with two running instances" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
//    entryHandler.connect(&engine); // disabled to stall process
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//   Execution::Recorder recorder(std::cerr);  // prints for debugging
    recorder.subscribe(&engine);

    engine.run(scenario.get(),0);
    const auto* originalState= engine.getSystemState();

    REQUIRE( originalState->instances.size() == 2 );
    REQUIRE( originalState->pendingEntryDecisions.count() == 2 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      //
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same number of instances" ) {
        REQUIRE( copiedState.instances.size() == originalState->instances.size() );
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
        REQUIRE( copiedState.globals.size() == originalState->globals.size() );
        REQUIRE( copiedState.contributionsToObjective == originalState->contributionsToObjective );
      }

      THEN( "The copy has the same pending entry decisions" ) {
        REQUIRE( copiedState.pendingEntryDecisions.count() == originalState->pendingEntryDecisions.count() );
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

    Model::DynamicDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Token should be waiting for ready event at Activity_1
    REQUIRE( originalState->tokensAwaitingReadyEvent.count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
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

  GIVEN( "An instance that stops waiting for exit event" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StochasticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    // First engine: entry handler but NO exit handler
    Execution::Engine engine1;
    Execution::InstantEntry entryHandler1;
    Execution::TimeWarp timeHandler1;
    entryHandler1.connect(&engine1);
    timeHandler1.connect(&engine1);
    Execution::Recorder recorder1;
    recorder1.subscribe(&engine1);

    // Run with timeout - will stop when task is waiting for exit event
    engine1.run(scenario.get(), 10);

    REQUIRE( engine1.getSystemState()->getTime() == 10 );

    // Token should be at COMPLETED state, waiting for exit event
    auto activityLog1 = recorder1.find(nlohmann::json{{"nodeId","Activity_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
    REQUIRE( activityLog1.back()["state"] == "COMPLETED" );

    WHEN( "A second engine resumes with exit handler and forked scenario" ) {
      // Fork scenario for continuation
      auto* stochasticScenario = dynamic_cast<Model::StochasticScenario*>(scenario.get());
      REQUIRE( stochasticScenario != nullptr );
      auto forkedScenario = std::make_unique<Model::StochasticScenario>(
        stochasticScenario,
        engine1.getSystemState()->getTime() + 1, // next point in time
        42  // new seed
      );

      // Second engine: has exit handler
      Execution::Engine engine2;
      Execution::InstantEntry entryHandler2;
      Execution::InstantExit exitHandler2;
      Execution::TimeWarp timeHandler2;
      entryHandler2.connect(&engine2);
      exitHandler2.connect(&engine2);
      timeHandler2.connect(&engine2);
      Execution::Recorder recorder2;
      recorder2.subscribe(&engine2);

      // Resume from first engine's state with forked scenario
      engine2.resume(forkedScenario.get(), engine1.getSystemState());

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
