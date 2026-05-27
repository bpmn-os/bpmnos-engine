SCENARIO( "SystemState copy with signal", "[systemstate][signal]" ) {
  const std::string modelFile = "tests/systemstate/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance waiting for a signal" ) {
    // Only instantiate Process_2 (receiver), not Process_1 (emitter)
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2; timestamp := 0\n"
    ;
    auto signalName = BPMNOS::to_number(std::string("Signal"),STRING);
    
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

    REQUIRE( originalState->tokensAwaitingSignal.count(signalName) == 1 );
    REQUIRE( originalState->tokensAwaitingSignal.at(signalName).count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting signal" ) {
        REQUIRE( copiedState.tokensAwaitingSignal.count(signalName) == 1 );
        REQUIRE( copiedState.tokensAwaitingSignal.at(signalName).count() ==
                 originalState->tokensAwaitingSignal.at(signalName).count() );
      }
    }
  }
}
