SCENARIO( "SystemState copy with parallel multi-instance activity", "[systemstate][multiinstanceactivity][parallel]" ) {
  const std::string modelFile = "tests/systemstate/multiinstanceactivity/Parallel_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with parallel multi-instance tokens" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    // No exit handler - tasks stay in BUSY state
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Should have 3 instance tokens and 1 main token (WAITING)
    REQUIRE( originalState->tokenAtMultiInstanceActivity.size() == 3 );
    REQUIRE( originalState->tokensAtActivityInstance.size() == 1 );
    auto& [mainToken, instanceTokens] = *originalState->tokensAtActivityInstance.begin();
    REQUIRE( instanceTokens.size() == 3 );
    REQUIRE( mainToken->state == Execution::Token::State::WAITING );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same multi-instance mappings" ) {
        REQUIRE( copiedState.tokenAtMultiInstanceActivity.size() ==
                 originalState->tokenAtMultiInstanceActivity.size() );
        REQUIRE( copiedState.tokensAtActivityInstance.size() ==
                 originalState->tokensAtActivityInstance.size() );
        auto& [copiedMainToken, copiedInstanceTokens] = *copiedState.tokensAtActivityInstance.begin();
        REQUIRE( copiedInstanceTokens.size() == instanceTokens.size() );
        REQUIRE( copiedMainToken->state == Execution::Token::State::WAITING );
      }
    }
  }
}

SCENARIO( "SystemState copy with sequential multi-instance activity", "[systemstate][multiinstanceactivity][sequential]" ) {
  const std::string modelFile = "tests/systemstate/multiinstanceactivity/Sequential_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with sequential multi-instance tokens" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    // No exit handler - first task stays in BUSY state, others wait
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Should have instance tokens and sequential waiting chain
    REQUIRE( originalState->tokenAtMultiInstanceActivity.size() == 3 );
    REQUIRE( originalState->tokensAtActivityInstance.size() == 1 );
    // Sequential: first token active, others waiting in chain
    REQUIRE( originalState->tokenAwaitingMultiInstanceExit.size() == 2 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same multi-instance mappings" ) {
        REQUIRE( copiedState.tokenAtMultiInstanceActivity.size() ==
                 originalState->tokenAtMultiInstanceActivity.size() );
        REQUIRE( copiedState.tokensAtActivityInstance.size() ==
                 originalState->tokensAtActivityInstance.size() );
        REQUIRE( copiedState.tokenAwaitingMultiInstanceExit.size() ==
                 originalState->tokenAwaitingMultiInstanceExit.size() );
      }
    }
  }
}
