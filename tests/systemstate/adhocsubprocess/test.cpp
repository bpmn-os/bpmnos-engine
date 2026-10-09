#include "prelude.h"

SCENARIO( "SystemState copy with SequentialAdHocSubProcess", "[systemstate][adhocsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/adhocsubprocess/AdHocSubProcess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with one activity performing and one pending" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    // No exit handler to keep activities alive
    entryHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    // Run without exit handler - first activity stays BUSY, second is pending
    dataProvider->setEndTime(0);
    engine.run(std::move(scenario), 0);
    const auto* originalState = engine.getSystemState();

    // Find the performer token (at AdHocSubProcess node)
    // Hierarchy: global state machine -> process token (node = process) -> owned -> AdHocSubProcess token
    Execution::Token* originalPerformerToken = nullptr;
    for (const auto& processToken : originalState->globalStateMachine->tokens) {
      if (processToken->owned) {
        for (const auto& token : processToken->owned->tokens) {
          if (token->node->id == "AdHocSubProcess_1") {
            originalPerformerToken = token.get();
            break;
          }
        }
      }
      if (originalPerformerToken) break;
    }
    REQUIRE( originalPerformerToken != nullptr );

    // Verify original state has performing and pendingSequentialEntries set
    REQUIRE( originalPerformerToken->performing != nullptr );
    REQUIRE( originalPerformerToken->pendingSequentialEntries.count() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same performing and pendingSequentialEntries" ) {
        // Find the copied performer token
        Execution::Token* copiedPerformerToken = nullptr;
        for (const auto& processToken : copiedState.globalStateMachine->tokens) {
          if (processToken->owned) {
            for (const auto& token : processToken->owned->tokens) {
              if (token->node->id == "AdHocSubProcess_1") {
                copiedPerformerToken = token.get();
                break;
              }
            }
          }
          if (copiedPerformerToken) break;
        }
        REQUIRE( copiedPerformerToken != nullptr );

        // Verify performing is copied (points to different token object but same node)
        REQUIRE( copiedPerformerToken->performing != nullptr );
        REQUIRE( copiedPerformerToken->performing != originalPerformerToken->performing );
        REQUIRE( copiedPerformerToken->performing->node == originalPerformerToken->performing->node );

        // Verify pendingSequentialEntries is copied
        REQUIRE( copiedPerformerToken->pendingSequentialEntries.count() ==
                 originalPerformerToken->pendingSequentialEntries.count() );
      }
    }
  }
}
