SCENARIO( "SystemState copy with compensation chain", "[systemstate][compensationactivity]" ) {
  const std::string modelFile = "tests/systemstate/compensationactivity/Two_compensations_triggered.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with compensation in progress" ) {
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

    // Run to time 5 - CompensationActivity_2 is in progress (completes at 10)
    engine.run(scenario.get(), 5);
    const auto* originalState = engine.getSystemState();

    // Should have compensation chain: compToken_2 -> compToken_1 -> triggeringToken
    REQUIRE( originalState->tokenAwaitingCompensationActivity.size() >= 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokenAwaitingCompensationActivity mappings" ) {
        REQUIRE( copiedState.tokenAwaitingCompensationActivity.size() ==
                 originalState->tokenAwaitingCompensationActivity.size() );
      }
    }
  }
}
