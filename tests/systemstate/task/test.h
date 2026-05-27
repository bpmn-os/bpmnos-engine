SCENARIO( "SystemState copy with token awaiting completion event", "[systemstate][task][completion]" ) {
  const std::string modelFile = "tests/systemstate/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with task completion time in the future" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
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
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Token should be waiting for completion at Activity_1 (completion time = 1)
    REQUIRE( originalState->tokensAwaitingCompletionEvent.count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting completion event" ) {
        REQUIRE( copiedState.tokensAwaitingCompletionEvent.count() ==
                 originalState->tokensAwaitingCompletionEvent.count() );
      }
    }
  }
}
