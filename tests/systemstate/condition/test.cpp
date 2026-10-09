#include "prelude.h"

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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    dataProvider->setEndTime(0);
    engine.run(std::move(scenario), 0);
    const auto* originalState = engine.getSystemState();

    // Instance ID for tokensAwaitingCondition key
    auto instanceId = originalState->globalStateMachine->tokens[0]->owned->instance.value();
    REQUIRE( originalState->tokensAwaitingCondition.count(instanceId) == 1 );
    REQUIRE( originalState->tokensAwaitingCondition.at(instanceId).count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting condition" ) {
        auto copiedInstanceId = copiedState.globalStateMachine->tokens[0]->owned->instance.value();
        REQUIRE( copiedState.tokensAwaitingCondition.count(copiedInstanceId) == 1 );
        REQUIRE( copiedState.tokensAwaitingCondition.at(copiedInstanceId).count() ==
                 originalState->tokensAwaitingCondition.at(instanceId).count() );
      }
    }
  }
}
