SCENARIO( "SystemState copy with tokens at event-based gateway", "[systemstate][eventbasedgateway]" ) {
  const std::string modelFile = "tests/systemstate/eventbasedgateway/Two_timer_events.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with tokens waiting at event-based gateway" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_1; Process_1; trigger1 := 10\n"
      "Instance_1; Process_1; trigger2 := 20\n"
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

    // Should have gateway token and 2 event tokens
    REQUIRE( originalState->tokensAwaitingEvent.size() == 1 );
    auto& [gatewayToken, eventTokens] = *originalState->tokensAwaitingEvent.begin();
    REQUIRE( eventTokens.size() == 2 );
    REQUIRE( originalState->tokenAtEventBasedGateway.size() == 2 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same event-based gateway mappings" ) {
        REQUIRE( copiedState.tokensAwaitingEvent.size() ==
                 originalState->tokensAwaitingEvent.size() );
        REQUIRE( copiedState.tokenAtEventBasedGateway.size() ==
                 originalState->tokenAtEventBasedGateway.size() );

        // Verify the gateway token has the same number of waiting event tokens
        auto& [copiedGatewayToken, copiedEventTokens] = *copiedState.tokensAwaitingEvent.begin();
        REQUIRE( copiedEventTokens.size() == eventTokens.size() );
      }
    }
  }
}
