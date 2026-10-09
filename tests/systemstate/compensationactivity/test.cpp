#include "prelude.h"

SCENARIO( "SystemState copy with compensation chain", "[systemstate][compensationactivity]" ) {
  const std::string modelFile = "tests/systemstate/compensationactivity/Two_compensations_triggered.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with compensation in progress" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
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

    // Run to time 5 - CompensationActivity_2 is in progress (completes at 10)
    dataProvider->setEndTime(5);
    engine.run(std::move(scenario), 0);
    const auto* originalState = engine.getSystemState();

    // Should have compensation chain: compToken_2 -> compToken_1 -> triggeringToken
    REQUIRE( originalState->tokenAwaitingCompensationActivity.size() >= 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokenAwaitingCompensationActivity mappings" ) {
        REQUIRE( copiedState.tokenAwaitingCompensationActivity.size() ==
                 originalState->tokenAwaitingCompensationActivity.size() );
      }
    }
  }
}
