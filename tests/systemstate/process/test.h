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
