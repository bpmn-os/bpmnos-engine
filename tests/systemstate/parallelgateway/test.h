SCENARIO( "SystemState copy with token awaiting gateway activation", "[systemstate][parallelgateway]" ) {
  const std::string modelFile = "tests/systemstate/parallelgateway/Fork-Join.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with token waiting at converging gateway" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    // No entry handler - task won't proceed, direct path token waits at gateway
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Should have 1 token waiting at Gateway_2
    REQUIRE( originalState->tokensAwaitingGatewayActivation.size() == 1 );
    auto& [sm, gatewayMap] = *originalState->tokensAwaitingGatewayActivation.begin();
    REQUIRE( gatewayMap.size() == 1 );
    auto& [gatewayNode, waitingTokens] = *gatewayMap.begin();
    REQUIRE( waitingTokens.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting gateway activation" ) {
        REQUIRE( copiedState.tokensAwaitingGatewayActivation.size() ==
                 originalState->tokensAwaitingGatewayActivation.size() );
        auto& [copiedSM, copiedGatewayMap] = *copiedState.tokensAwaitingGatewayActivation.begin();
        REQUIRE( copiedGatewayMap.size() == gatewayMap.size() );
        auto& [copiedGatewayNode, copiedWaitingTokens] = *copiedGatewayMap.begin();
        REQUIRE( copiedWaitingTokens.size() == waitingTokens.size() );
        REQUIRE( copiedGatewayNode == gatewayNode );
      }
    }
  }
}
