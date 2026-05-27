SCENARIO( "SystemState copy with token awaiting condition", "[systemstate][condition]" ) {
  const std::string modelFile = "tests/systemstate/condition/Condition_on_global.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance waiting for a condition" ) {
    // Only instantiate Process_2 (with conditional event), not Process_1 (which changes condition)
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := false\n"
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

    // Instance ID for tokensAwaitingCondition key
    auto instanceId = originalState->instances[0]->instance.value();
    REQUIRE( originalState->tokensAwaitingCondition.count(instanceId) == 1 );
    REQUIRE( originalState->tokensAwaitingCondition.at(instanceId).count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting condition" ) {
        auto copiedInstanceId = copiedState.instances[0]->instance.value();
        REQUIRE( copiedState.tokensAwaitingCondition.count(copiedInstanceId) == 1 );
        REQUIRE( copiedState.tokensAwaitingCondition.at(copiedInstanceId).count() ==
                 originalState->tokensAwaitingCondition.at(instanceId).count() );
      }
    }
  }
}
