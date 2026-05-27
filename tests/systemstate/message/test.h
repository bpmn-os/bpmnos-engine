SCENARIO( "SystemState copy with token awaiting boundary event", "[systemstate][message][boundary]" ) {
  const std::string modelFile = "tests/systemstate/message/Message_tasks_with_timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with a receive task and timer boundary event" ) {
    // Only instantiate Process_2 (receiver with timer boundary event)
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2; timestamp := 0\n"
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

    // Should have boundary event token associated with activity token
    REQUIRE( originalState->tokenAssociatedToBoundaryEventToken.size() == 1 );
    REQUIRE( originalState->tokensAwaitingBoundaryEvent.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same boundary event mappings" ) {
        REQUIRE( copiedState.tokenAssociatedToBoundaryEventToken.size() ==
                 originalState->tokenAssociatedToBoundaryEventToken.size() );
        REQUIRE( copiedState.tokensAwaitingBoundaryEvent.size() ==
                 originalState->tokensAwaitingBoundaryEvent.size() );
      }
    }
  }
}
