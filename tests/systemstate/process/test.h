SCENARIO( "SystemState copy for simple process", "[systemstate]" ) {
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

    REQUIRE( originalState->instances.size() == 2);

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
    }
  }
}
